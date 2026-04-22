#include "MacCaptureBackend.h"
#include "MacPlatform.h"

#include <QDebug>
#include <QFile>
#include <QMetaObject>
#include <QUrl>

#import <AppKit/NSWorkspace.h>
#import <AVFoundation/AVFoundation.h>
#import <CoreGraphics/CoreGraphics.h>
#import <ScreenCaptureKit/ScreenCaptureKit.h>

using namespace screencopy;

// ---------- SCStream + SCRecordingOutput delegate ----------

API_AVAILABLE(macos(15.0))
@interface SCKRecordingDelegate : NSObject <SCStreamDelegate, SCRecordingOutputDelegate>
@property (nonatomic, assign) MacCaptureBackend *backend;
@property (nonatomic, copy) NSString *outputPath;
@end

@implementation SCKRecordingDelegate

// SCStreamDelegate — stream-level errors
- (void)stream:(SCStream *)stream didStopWithError:(NSError *)error
{
    if (!self.backend)
        return;

    QString msg = QString::fromNSString(error.localizedDescription);
    auto *backend = self.backend;
    QMetaObject::invokeMethod(backend, [backend, msg]() {
        emit backend->recordingError(msg);
    }, Qt::QueuedConnection);
}

// SCRecordingOutputDelegate
- (void)recordingOutputDidStartRecording:(SCRecordingOutput *)output
{
    if (!self.backend)
        return;

    auto *backend = self.backend;
    QMetaObject::invokeMethod(backend, [backend]() {
        emit backend->recordingStarted(QString());
    }, Qt::QueuedConnection);
}

- (void)recordingOutput:(SCRecordingOutput *)output didFailWithError:(NSError *)error
{
    if (!self.backend)
        return;

    QString msg = QString::fromNSString(error.localizedDescription);
    auto *backend = self.backend;
    QMetaObject::invokeMethod(backend, [backend, msg]() {
        emit backend->recordingError(msg);
    }, Qt::QueuedConnection);
}

- (void)recordingOutputDidFinishRecording:(SCRecordingOutput *)output
{
    if (!self.backend)
        return;

    // File is fully written — now stop the stream and notify
    auto *backend = self.backend;
    QString path = QString::fromNSString(self.outputPath);
    QMetaObject::invokeMethod(backend, [backend, path]() {
        backend->finishRecording(path);
    }, Qt::QueuedConnection);
}

@end

// ---------- Helper: build SCContentFilter from source ID (async) ----------

typedef void (^FilterCompletionBlock)(SCContentFilter * _Nullable filter);

static void filterFromSourceIdAsync(NSString *sourceId, FilterCompletionBlock completion, void (^errorHandler)(NSError *error)) API_AVAILABLE(macos(15.0))
{
    [SCShareableContent getShareableContentExcludingDesktopWindows:NO
                                              onScreenWindowsOnly:NO
                                                completionHandler:^(SCShareableContent *content, NSError *error) {
        if (!content || error) {
            if (errorHandler && error)
                errorHandler(error);
            else
                completion(nil);
            return;
        }

        // Parse source ID: "display:<id>", "window:<id>", "area:x,y,wxh"
        NSString *prefix = nil;
        NSString *value = nil;
        NSRange colonRange = [sourceId rangeOfString:@":"];
        if (colonRange.location != NSNotFound) {
            prefix = [sourceId substringToIndex:colonRange.location];
            value = [sourceId substringFromIndex:colonRange.location + 1];
        }

        SCContentFilter *filter = nil;

        if ([prefix isEqualToString:@"display"]) {
            uint32_t displayID = (uint32_t)[value integerValue];
            for (SCDisplay *display in content.displays) {
                if (display.displayID == displayID) {
                    filter = [[SCContentFilter alloc] initWithDisplay:display
                                                excludingApplications:@[]
                                                    exceptingWindows:@[]];
                    break;
                }
            }
            if (!filter && content.displays.count > 0) {
                filter = [[SCContentFilter alloc] initWithDisplay:content.displays.firstObject
                                            excludingApplications:@[]
                                                exceptingWindows:@[]];
            }
        } else if ([prefix isEqualToString:@"window"]) {
            uint32_t windowID = (uint32_t)[value integerValue];
            for (SCWindow *window in content.windows) {
                if (window.windowID == windowID) {
                    filter = [[SCContentFilter alloc] initWithDesktopIndependentWindow:window];
                    break;
                }
            }
        } else if ([prefix isEqualToString:@"area"]) {
            if (content.displays.count > 0) {
                filter = [[SCContentFilter alloc] initWithDisplay:content.displays.firstObject
                                            excludingApplications:@[]
                                                exceptingWindows:@[]];
            }
        }

        // Last resort: primary display
        if (!filter && content.displays.count > 0) {
            filter = [[SCContentFilter alloc] initWithDisplay:content.displays.firstObject
                                        excludingApplications:@[]
                                            exceptingWindows:@[]];
        }

        completion(filter);
    }];
}

// ---------- MacCaptureBackend ----------

namespace screencopy {

MacCaptureBackend::MacCaptureBackend(PlatformIntegration *platform, QObject *parent)
    : CaptureBackend(parent)
    , m_platform(platform)
{
    // Request screen recording access early so the app appears in System Settings.
    // This is a no-op if already granted.
    CGRequestScreenCaptureAccess();

    if (@available(macOS 15.0, *)) {
        SCKRecordingDelegate *delegate = [[SCKRecordingDelegate alloc] init];
        delegate.backend = this;
        m_delegate = (__bridge_retained void *)delegate;
    }
}

MacCaptureBackend::~MacCaptureBackend()
{
    cleanup();
    if (m_delegate) {
        if (@available(macOS 15.0, *)) {
            SCKRecordingDelegate *delegate = (__bridge_transfer SCKRecordingDelegate *)m_delegate;
            delegate.backend = nullptr;
            (void)delegate;
        }
    }
}

bool MacCaptureBackend::isAvailable() const
{
    if (@available(macOS 15.0, *))
        return true;
    return false;
}

void MacCaptureBackend::startRecording(const QString &outputPath)
{
    startRecording(outputPath, RecordingOptions{});
}

void MacCaptureBackend::startRecording(const QString &outputPath, const RecordingOptions &options)
{
    if (m_state != RecordingState::Idle)
        return;

    if (@available(macOS 15.0, *)) {
        m_outputPath = outputPath;
        m_state = RecordingState::Starting;
        emit stateChanged(m_state);

        // Try to use the filter stored from source selection (avoids re-enumerating)
        auto *macPlatform = qobject_cast<MacPlatform *>(m_platform);
        void *storedFilter = macPlatform ? macPlatform->takeLastFilter() : nullptr;

        if (storedFilter) {
            startStream(storedFilter, options, outputPath);
        } else {
            // Fallback: create filter from source ID string
            NSString *sourceId = options.windowMode.toNSString();
            RecordingOptions capturedOptions = options;
            QString capturedPath = outputPath;

            filterFromSourceIdAsync(sourceId,
            ^(SCContentFilter *filter) {
                void *retainedFilter = filter ? (__bridge_retained void *)filter : nullptr;
                RecordingOptions opts = capturedOptions;
                QString path = capturedPath;

                QMetaObject::invokeMethod(this, [this, retainedFilter, opts, path]() {
                    if (@available(macOS 15.0, *)) {
                        SCContentFilter *f = retainedFilter
                            ? (__bridge_transfer SCContentFilter *)retainedFilter : nil;
                        if (!f) {
                            m_state = RecordingState::Idle;
                            emit stateChanged(m_state);
                            emit recordingError("Failed to create capture filter — check Screen Recording permission");
                            return;
                        }
                        startStream((__bridge_retained void *)f, opts, path);
                    }
                }, Qt::QueuedConnection);
            },
            ^(NSError *error) {
                QString msg = QString::fromNSString(error.localizedDescription);
                QMetaObject::invokeMethod(this, [this, msg]() {
                    m_state = RecordingState::Idle;
                    emit stateChanged(m_state);
                    emit recordingError(msg);
                    [[NSWorkspace sharedWorkspace]
                        openURL:[NSURL URLWithString:@"x-apple.systempreferences:com.apple.preference.security?Privacy_ScreenCapture"]];
                }, Qt::QueuedConnection);
            });
        }
    } else {
        emit recordingError("Screen recording requires macOS 15.0 or later");
    }
}

void MacCaptureBackend::startStream(void *retainedFilter, const RecordingOptions &options, const QString &outputPath)
{
    if (@available(macOS 15.0, *)) {
        SCContentFilter *filter = (__bridge_transfer SCContentFilter *)retainedFilter;

        SCStreamConfiguration *config = [[SCStreamConfiguration alloc] init];
        config.minimumFrameInterval = CMTimeMake(1, options.frameRate);
        config.showsCursor = YES;
        config.capturesAudio = options.systemAudio;
        config.excludesCurrentProcessAudio = YES;
        config.sampleRate = 48000;
        config.channelCount = 2;

        if (!options.captureRegion.isNull()) {
            QRect r = options.captureRegion;
            config.sourceRect = CGRectMake(r.x(), r.y(), r.width(), r.height());
            config.width = r.width() * 2;
            config.height = r.height() * 2;
            config.scalesToFit = YES;
        }

        SCKRecordingDelegate *delegate = (__bridge SCKRecordingDelegate *)m_delegate;
        delegate.outputPath = outputPath.toNSString();
        SCStream *stream = [[SCStream alloc] initWithFilter:filter
                                              configuration:config
                                                   delegate:delegate];

        NSURL *fileURL = [NSURL fileURLWithPath:outputPath.toNSString()];
        SCRecordingOutputConfiguration *recordingConfig =
            [[SCRecordingOutputConfiguration alloc] init];
        recordingConfig.outputURL = fileURL;
        recordingConfig.outputFileType = AVFileTypeMPEG4;

        SCRecordingOutput *recordingOutput =
            [[SCRecordingOutput alloc] initWithConfiguration:recordingConfig
                                                   delegate:delegate];

        NSError *error = nil;
        [stream addRecordingOutput:recordingOutput error:&error];
        if (error) {
            m_state = RecordingState::Idle;
            emit stateChanged(m_state);
            emit recordingError(QString::fromNSString(error.localizedDescription));
            return;
        }

        m_stream = (__bridge_retained void *)stream;
        m_recordingOutput = (__bridge_retained void *)recordingOutput;

        [stream startCaptureWithCompletionHandler:^(NSError *startError) {
            if (startError) {
                QString msg = QString::fromNSString(startError.localizedDescription);
                QMetaObject::invokeMethod(this, [this, msg]() {
                    cleanup();
                    m_state = RecordingState::Idle;
                    emit stateChanged(m_state);
                    emit recordingError(msg);
                }, Qt::QueuedConnection);
            } else {
                QMetaObject::invokeMethod(this, [this]() {
                    m_state = RecordingState::Recording;
                    emit stateChanged(m_state);
                    emit recordingStarted(m_outputPath);
                }, Qt::QueuedConnection);
            }
        }];
    }
}

void MacCaptureBackend::stopRecording()
{
    if (m_state != RecordingState::Recording && m_state != RecordingState::Paused)
        return;

    m_state = RecordingState::Stopping;
    emit stateChanged(m_state);

    if (@available(macOS 15.0, *)) {
        SCStream *stream = (__bridge SCStream *)m_stream;

        // Remove recording output to trigger file finalization.
        // The delegate's recordingOutputDidFinishRecording: will call
        // finishRecording() once the file is fully written.
        if (m_recordingOutput) {
            SCRecordingOutput *output = (__bridge SCRecordingOutput *)m_recordingOutput;
            NSError *removeError = nil;
            [stream removeRecordingOutput:output error:&removeError];
        }
    }
}

void MacCaptureBackend::finishRecording(const QString &path)
{
    if (@available(macOS 15.0, *)) {
        if (m_stream) {
            SCStream *stream = (__bridge SCStream *)m_stream;
            [stream stopCaptureWithCompletionHandler:^(NSError *error) {
                QMetaObject::invokeMethod(this, [this, path]() {
                    cleanup();
                    m_state = RecordingState::Idle;
                    emit stateChanged(m_state);
                    emit recordingStopped(path);
                }, Qt::QueuedConnection);
            }];
        } else {
            cleanup();
            m_state = RecordingState::Idle;
            emit stateChanged(m_state);
            emit recordingStopped(path);
        }
    }
}

void MacCaptureBackend::pauseRecording()
{
    // SCStream doesn't have native pause — we keep the stream running
    // and note the paused state for the UI
    if (m_state != RecordingState::Recording)
        return;

    m_state = RecordingState::Paused;
    emit stateChanged(m_state);
}

void MacCaptureBackend::resumeRecording()
{
    if (m_state != RecordingState::Paused)
        return;

    m_state = RecordingState::Recording;
    emit stateChanged(m_state);
}

void MacCaptureBackend::cancelRecording()
{
    if (@available(macOS 15.0, *)) {
        if (m_stream) {
            SCStream *stream = (__bridge SCStream *)m_stream;
            [stream stopCaptureWithCompletionHandler:^(NSError *error) {
                Q_UNUSED(error);
            }];
        }
    }

    QString path = m_outputPath;
    cleanup();
    m_state = RecordingState::Idle;
    emit stateChanged(m_state);

    if (!path.isEmpty())
        QFile::remove(path);
}

void MacCaptureBackend::cleanup()
{
    if (@available(macOS 15.0, *)) {
        if (m_recordingOutput) {
            (void)(__bridge_transfer SCRecordingOutput *)m_recordingOutput;
            m_recordingOutput = nullptr;
        }
        if (m_stream) {
            (void)(__bridge_transfer SCStream *)m_stream;
            m_stream = nullptr;
        }
    }
    m_outputPath.clear();
}

} // namespace screencopy
