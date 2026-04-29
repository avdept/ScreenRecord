#pragma once

#include "CaptureBackend.h"

namespace screencopy {

class PipeWireCapture : public CaptureBackend
{
    Q_OBJECT

public:
    explicit PipeWireCapture(QObject *parent = nullptr);

    RecordingState state() const override { return m_state; }
    void startRecording(const QString &outputPath, const RecordingOptions &options) override;
    void stopRecording() override;
    void pauseRecording() override;
    void resumeRecording() override;
    void cancelRecording() override;
    bool isAvailable() const override;

private:
    RecordingState m_state = RecordingState::Idle;
};

} // namespace screencopy
