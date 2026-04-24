#include "MacPlatform.h"
#include "MacScreenCapture.h"
#include "MacScreenCaptureBridge.h"

#include <QWindow>

namespace screencopy {

MacPlatform::MacPlatform(QObject *parent)
    : PlatformIntegration(parent)
    , m_screenCapture(new MacScreenCapture(this))
{
    connect(m_screenCapture, &MacScreenCapture::sourceSelected,
            this, &PlatformIntegration::sourceSelected);
    connect(m_screenCapture, &MacScreenCapture::selectionCancelled,
            this, &PlatformIntegration::sourceSelectionCancelled);
    connect(m_screenCapture, &MacScreenCapture::selectionError,
            this, &PlatformIntegration::sourceSelectionError);
}

void MacPlatform::requestSourceSelection()
{
    m_screenCapture->showPicker();
}

void MacPlatform::selectFullScreen()
{
    m_screenCapture->showDisplayPicker();
}

void MacPlatform::selectWindow()
{
    m_screenCapture->showWindowPicker();
}

void *MacPlatform::takeLastFilter()
{
    return m_screenCapture->takeLastFilter();
}

void MacPlatform::applyHudWindowRulesImpl(QWindow *window)
{
    if (!window)
        return;

    window->setFlags(window->flags()
                     | Qt::FramelessWindowHint
                     | Qt::WindowStaysOnTopHint
                     | Qt::Tool);

    // Promote to status-bar level + all-Spaces collection behavior so the HUD
    // stays accessible when the user records a fullscreen app. Qt's
    // WindowStaysOnTopHint alone isn't enough on macOS.
    if (WId winId = window->winId())
        sc_hud_apply_window_rules(reinterpret_cast<void *>(winId));
}

} // namespace screencopy
