#pragma once

#include <QObject>

namespace screencopy {

enum class WindowMode {
    Hud,
    Editor,
    SourceSelector
};

class WindowManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString currentView READ currentView NOTIFY currentViewChanged)

public:
    explicit WindowManager(QObject *parent = nullptr);

    QString currentView() const { return m_currentView; }

    Q_INVOKABLE void switchToHud();
    Q_INVOKABLE void switchToEditor();
    Q_INVOKABLE void showSourceSelector();

signals:
    void currentViewChanged();

private:
    QString m_currentView = "editor";
};

} // namespace screencopy
