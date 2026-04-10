#include "Application.h"

namespace screencopy {

Application::Application(QObject *parent)
    : QObject(parent)
{
}

void Application::setupSystemTray()
{
    // TODO: System tray integration
    // Qt Quick apps can use platform-specific tray via StatusNotifierItem on Linux
    // or add Qt::Widgets dependency for QSystemTrayIcon
}

} // namespace screencopy
