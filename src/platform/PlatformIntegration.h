#pragma once

#include <QObject>
#include <QString>
#include <QWindow>

namespace screencopy {

class PlatformIntegration : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isWayland READ isWayland CONSTANT)
    Q_PROPERTY(bool isHyprland READ isHyprland CONSTANT)
    Q_PROPERTY(QString platformName READ platformName CONSTANT)

public:
    explicit PlatformIntegration(QObject *parent = nullptr);

    static PlatformIntegration *create(QObject *parent = nullptr);

    // Platform identity
    virtual bool isWayland() const { return false; }
    virtual bool isHyprland() const { return false; }
    virtual QString platformName() const = 0;

    // Window management — callable from QML with an ApplicationWindow
    Q_INVOKABLE virtual void applyHudWindowRules(QObject *window);

    // File system
    virtual QString recordingsDir() const;

    // Source selection — shows the native screen/window picker.
    Q_INVOKABLE virtual void requestSourceSelection() = 0;

    // Recording — platform-specific screen capture.
    Q_INVOKABLE virtual void startRecording(const QString &outputPath);
    Q_INVOKABLE virtual void stopRecording();
    Q_INVOKABLE virtual void pauseRecording();
    Q_INVOKABLE virtual void resumeRecording();
    Q_INVOKABLE virtual void cancelRecording();
    virtual bool supportsNativeRecording() const { return false; }

    // Utilities
    Q_INVOKABLE void revealInFolder(const QString &filePath);
    Q_INVOKABLE void openExternalUrl(const QString &url);

protected:
    virtual void applyHudWindowRulesImpl(QWindow *window) { Q_UNUSED(window); }

signals:
    void sourceSelected(const QString &sourceId, const QString &sourceName);
    void sourceSelectionCancelled();
    void sourceSelectionError(const QString &message);

    void recordingStarted(const QString &outputPath);
    void recordingStopped(const QString &outputPath);
    void recordingError(const QString &message);
};

} // namespace screencopy
