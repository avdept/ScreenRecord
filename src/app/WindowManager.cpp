#include "WindowManager.h"

namespace screencopy {

WindowManager::WindowManager(QObject *parent)
    : QObject(parent)
{
}

void WindowManager::switchToHud()
{
    m_currentView = "hud";
    emit currentViewChanged();
}

void WindowManager::switchToEditor()
{
    m_currentView = "editor";
    emit currentViewChanged();
}

void WindowManager::showSourceSelector()
{
    m_currentView = "sourceSelector";
    emit currentViewChanged();
}

} // namespace screencopy
