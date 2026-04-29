#pragma once

#include "CaptureBackend.h"
#include <QProcess>
#include <QTimer>

namespace screencopy {

class PlatformIntegration;

// Wraps the external `gpu-screen-recorder` binary, used as the Linux capture
// engine. Source selection is delegated to the XDG ScreenCast portal, which
// gpu-screen-recorder invokes itself when started — so the user picker comes
// up at start time rather than ahead of time.
class LinuxCaptureBackend : public CaptureBackend
{
    Q_OBJECT

public:
    explicit LinuxCaptureBackend(PlatformIntegration *platform, QObject *parent = nullptr);
    ~LinuxCaptureBackend() override;

    RecordingState state() const override { return m_state; }
    void startRecording(const QString &outputPath, const RecordingOptions &options) override;
    void stopRecording() override;
    void pauseRecording() override;
    void resumeRecording() override;
    void cancelRecording() override;
    bool isAvailable() const override;
    bool requiresSourceSelection() const override { return false; }

    QString outputPath() const { return m_outputPath; }

private:
    void killIfRunning();

    PlatformIntegration *m_platform = nullptr;
    QProcess *m_process = nullptr;
    RecordingState m_state = RecordingState::Idle;
    QString m_outputPath;
    QTimer m_startTimeout;
    QTimer *m_pollTimer = nullptr;
};

} // namespace screencopy
