#pragma once

#include "CaptureBackend.h"

namespace screencopy {

class PlatformIntegration;

// C++/Qt façade over the Swift-side ScreenCaptureKit recording engine
// (see MacCaptureBridge.h and src/capture/mac/).
class MacCaptureBackend : public CaptureBackend
{
    Q_OBJECT

public:
    explicit MacCaptureBackend(PlatformIntegration *platform, QObject *parent = nullptr);
    ~MacCaptureBackend() override;

    RecordingState state() const override { return m_state; }

    void startRecording(const QString &outputPath, const RecordingOptions &options) override;
    void stopRecording() override;
    void pauseRecording() override;
    void resumeRecording() override;
    void cancelRecording() override;
    bool isAvailable() const override;

    // macOS recording uses HEVC-with-alpha in a QuickTime container to
    // preserve rounded-window corner transparency.
    QString preferredExtension() const override { return QStringLiteral("mov"); }

private:
    // Static C trampolines — called from Swift on SCK queues.
    static void onStarted(void *ctx, const char *outputPath);
    static void onStopped(void *ctx, const char *outputPath);
    static void onError(void *ctx, const char *message);

    PlatformIntegration *m_platform = nullptr;
    RecordingState m_state = RecordingState::Idle;
    QString m_outputPath;
    void *m_recorder = nullptr; // SCRecorderHandle — Swift-owned
};

} // namespace screencopy
