#pragma once

#include <QObject>

namespace screencopy {

class Application : public QObject
{
    Q_OBJECT

public:
    explicit Application(QObject *parent = nullptr);

    // System tray requires Qt Widgets — will be set up when needed
    void setupSystemTray();

signals:
    void trayActivated();
    void quitRequested();
};

} // namespace screencopy
