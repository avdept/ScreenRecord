#pragma once

#include "CaptureBackend.h"
#include "GpuScreenRecorder.h" // for RecordingOptions

namespace screencopy {

class PlatformIntegration;

class MacCaptureBackend : public CaptureBackend
{
    Q_OBJECT

public:
    explicit MacCaptureBackend(PlatformIntegration *platform, QObject *parent = nullptr);
    ~MacCaptureBackend() override;

    RecordingState state() const override { return m_state; }

    void startRecording(const QString &outputPath) override;
    void startRecording(const QString &outputPath, const RecordingOptions &options);
    void stopRecording() override;
    void pauseRecording() override;
    void resumeRecording() override;
    void cancelRecording() override;
    bool isAvailable() const override;

    void finishRecording(const QString &path);

private:
    void startStream(void *retainedFilter, const RecordingOptions &options, const QString &outputPath);
    void cleanup();

    PlatformIntegration *m_platform = nullptr;
    RecordingState m_state = RecordingState::Idle;
    QString m_outputPath;
    void *m_stream = nullptr;          // SCStream*
    void *m_recordingOutput = nullptr; // SCRecordingOutput*
    void *m_delegate = nullptr;        // ObjC delegate
};

} // namespace screencopy
