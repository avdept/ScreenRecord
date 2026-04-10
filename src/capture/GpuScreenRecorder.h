#pragma once

#include "CaptureBackend.h"
#include <QProcess>
#include <QTimer>

namespace screencopy {

class PlatformIntegration;

struct RecordingOptions {
    bool systemAudio = true;
    bool microphone = false;
    int frameRate = 60;
    QString windowMode = "portal";  // "portal", "focused", or a window ID
};

class GpuScreenRecorder : public CaptureBackend
{
    Q_OBJECT

public:
    explicit GpuScreenRecorder(PlatformIntegration *platform, QObject *parent = nullptr);
    ~GpuScreenRecorder() override;

    RecordingState state() const override { return m_state; }
    void startRecording(const QString &outputPath) override;
    void startRecording(const QString &outputPath, const RecordingOptions &options);
    void stopRecording() override;
    void pauseRecording() override;
    void resumeRecording() override;
    void cancelRecording() override;
    bool isAvailable() const override;

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
