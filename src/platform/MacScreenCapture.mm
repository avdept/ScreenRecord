#include "MacScreenCapture.h"

#include <QDebug>
#include <QMetaObject>

#import <ScreenCaptureKit/ScreenCaptureKit.h>

using namespace screencopy;

// ---------- Native picker observer (macOS 14+) ----------

API_AVAILABLE(macos(14.0))
@interface SCKPickerHelper : NSObject <SCContentSharingPickerObserver>
@property (nonatomic, assign) MacScreenCapture *qtCapture;
@end

@implementation SCKPickerHelper

- (void)contentSharingPicker:(SCContentSharingPicker *)picker
         didUpdateWithFilter:(SCContentFilter *)filter
                   forStream:(nullable SCStream *)stream
{
    if (!self.qtCapture)
        return;

    auto *capture = self.qtCapture;

    // Store the filter for direct use by the recording backend
    void *retainedFilter = (__bridge_retained void *)filter;

    CGRect filterRect = filter.contentRect;
    int fw = (int)CGRectGetWidth(filterRect);
    int fh = (int)CGRectGetHeight(filterRect);

    // Build a display name from the filter dimensions
    QString sourceId = QString("filter:%1x%2").arg(fw).arg(fh);
    QString sourceName = QString("Selection (%1\u00d7%2)").arg(fw).arg(fh);

    QMetaObject::invokeMethod(capture, [capture, retainedFilter, sourceId, sourceName]() {
        if (@available(macOS 14.0, *)) {
            // Store the filter on the capture object
            capture->storeFilter(retainedFilter);
        }
        emit capture->sourceSelected(sourceId, sourceName);
    }, Qt::QueuedConnection);

    picker.active = NO;
    [picker removeObserver:self];
}

- (void)contentSharingPicker:(SCContentSharingPicker *)picker
          didCancelForStream:(nullable SCStream *)stream
{
    if (!self.qtCapture)
        return;

    auto *capture = self.qtCapture;
    QMetaObject::invokeMethod(capture, [capture]() {
        emit capture->selectionCancelled();
    }, Qt::QueuedConnection);

    picker.active = NO;
    [picker removeObserver:self];
}

- (void)contentSharingPickerStartDidFailWithError:(NSError *)error
{
    if (!self.qtCapture)
        return;

    QString msg = QString::fromNSString(error.localizedDescription);
    auto *capture = self.qtCapture;
    QMetaObject::invokeMethod(capture, [capture, msg]() {
        emit capture->selectionError(msg);
    }, Qt::QueuedConnection);
}

@end

// ---------- MacScreenCapture ----------

namespace screencopy {

MacScreenCapture::MacScreenCapture(QObject *parent)
    : QObject(parent)
{
    if (@available(macOS 14.0, *)) {
        SCKPickerHelper *helper = [[SCKPickerHelper alloc] init];
        helper.qtCapture = this;
        m_helper = (__bridge_retained void *)helper;
    }
}

MacScreenCapture::~MacScreenCapture()
{
    if (m_helper) {
        if (@available(macOS 14.0, *)) {
            SCKPickerHelper *helper = (__bridge_transfer SCKPickerHelper *)m_helper;
            helper.qtCapture = nullptr;
            (void)helper; // ARC releases
        }
    }
}

// Shared helper: presents the native picker with the given mode flags (macOS 14+)
void MacScreenCapture::presentPickerWithModes(unsigned long modes)
{
    if (@available(macOS 14.0, *)) {
        SCKPickerHelper *helper = (__bridge SCKPickerHelper *)m_helper;
        SCContentSharingPicker *picker = [SCContentSharingPicker sharedPicker];

        if (picker.isActive) {
            picker.active = NO;
            [picker removeObserver:helper];
        }

        SCContentSharingPickerConfiguration *config =
            [[SCContentSharingPickerConfiguration alloc] init];
        config.allowedPickerModes = (SCContentSharingPickerMode)modes;
        picker.defaultConfiguration = config;

        [picker addObserver:helper];
        picker.active = YES;
        [picker present];
    }
}

void MacScreenCapture::storeFilter(void *retainedFilter)
{
    // Release any previously stored filter
    if (m_lastFilter) {
        if (@available(macOS 12.3, *)) {
            (void)(__bridge_transfer SCContentFilter *)m_lastFilter;
        }
    }
    m_lastFilter = retainedFilter;
}

void *MacScreenCapture::takeLastFilter()
{
    void *f = m_lastFilter;
    m_lastFilter = nullptr;
    return f;
}

void MacScreenCapture::showPicker()
{
    if (@available(macOS 14.0, *)) {
        presentPickerWithModes(
            SCContentSharingPickerModeSingleDisplay | SCContentSharingPickerModeSingleWindow);
        return;
    }

    // macOS 12.3–13: enumerate displays, auto-select the main one
    enumerateAndSelectMainDisplay();
}

void MacScreenCapture::showDisplayPicker()
{
    if (@available(macOS 14.0, *)) {
        // For single display, skip the picker and enumerate directly
        enumerateAndSelectMainDisplay();
        return;
    }

    enumerateAndSelectMainDisplay();
}

void MacScreenCapture::showWindowPicker()
{
    if (@available(macOS 14.0, *)) {
        presentPickerWithModes(SCContentSharingPickerModeSingleWindow);
        return;
    }

    // macOS 12.3–13: enumerate windows, select the frontmost non-self window
    if (@available(macOS 12.3, *)) {
        auto *self = this;
        [SCShareableContent getShareableContentExcludingDesktopWindows:YES
                                                  onScreenWindowsOnly:YES
                                                    completionHandler:^(SCShareableContent *content,
                                                                        NSError *error) {
            if (error || !content || content.windows.count == 0) {
                QString msg = error ? QString::fromNSString(error.localizedDescription)
                                    : QStringLiteral("No windows found");
                QMetaObject::invokeMethod(self, [self, msg]() {
                    emit self->selectionError(msg);
                }, Qt::QueuedConnection);
                return;
            }

            // Pick the first on-screen window that has a title
            SCWindow *target = nil;
            for (SCWindow *window in content.windows) {
                if (window.owningApplication && window.title.length > 0) {
                    target = window;
                    break;
                }
            }
            if (!target)
                target = content.windows.firstObject;

            NSString *appName = target.owningApplication.applicationName ?: @"Unknown";
            NSString *title = target.title;
            QString sourceId = QString("window:%1").arg(target.windowID);
            QString sourceName = (title.length > 0)
                ? QString("%1 \u2013 %2").arg(QString::fromNSString(appName),
                                              QString::fromNSString(title))
                : QString::fromNSString(appName);

            QMetaObject::invokeMethod(self, [self, sourceId, sourceName]() {
                emit self->sourceSelected(sourceId, sourceName);
            }, Qt::QueuedConnection);
        }];
        return;
    }

    emit selectionError(QStringLiteral("Window selection requires macOS 12.3 or later"));
}

// Enumerates displays and selects the main one
void MacScreenCapture::enumerateAndSelectMainDisplay()
{
    if (@available(macOS 12.3, *)) {
        auto *self = this;
        [SCShareableContent getShareableContentExcludingDesktopWindows:NO
                                                  onScreenWindowsOnly:NO
                                                    completionHandler:^(SCShareableContent *content,
                                                                        NSError *error) {
            if (error || !content || content.displays.count == 0) {
                QString msg = error ? QString::fromNSString(error.localizedDescription)
                                    : QStringLiteral("No displays found");
                QMetaObject::invokeMethod(self, [self, msg]() {
                    emit self->selectionError(msg);
                }, Qt::QueuedConnection);
                return;
            }

            SCDisplay *display = content.displays.firstObject;
            SCContentFilter *filter = [[SCContentFilter alloc] initWithDisplay:display
                                                         excludingApplications:@[]
                                                             exceptingWindows:@[]];
            void *retainedFilter = (__bridge_retained void *)filter;

            QString sourceId = QString("display:%1").arg(display.displayID);
            QString sourceName =
                QString("Full Screen (%1\u00d7%2)").arg(display.width).arg(display.height);

            QMetaObject::invokeMethod(self, [self, retainedFilter, sourceId, sourceName]() {
                self->storeFilter(retainedFilter);
                emit self->sourceSelected(sourceId, sourceName);
            }, Qt::QueuedConnection);
        }];
        return;
    }

    emit selectionError(QStringLiteral("Screen capture requires macOS 12.3 or later"));
}

} // namespace screencopy
