#include "MacPlatform.h"
#include "MacScreenCapture.h"

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
}

} // namespace screencopy
