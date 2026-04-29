#pragma once

#include <QObject>
#include <QString>

namespace screencopy {

// Recording state machine
enum class RecordingState {
    Idle,
    Starting,
    Recording,
    Paused,
    Stopping
};

class CaptureBackend : public QObject
{
    Q_OBJECT
    Q_PROPERTY(RecordingState state READ state NOTIFY stateChanged)

public:
    explicit CaptureBackend(QObject *parent = nullptr) : QObject(parent) {}

    virtual RecordingState state() const = 0;

    virtual void startRecording(const QString &outputPath) = 0;
    virtual void stopRecording() = 0;
    virtual void pauseRecording() = 0;
    virtual void resumeRecording() = 0;
    virtual void cancelRecording() = 0;
    virtual bool isAvailable() const = 0;

    // File extension the backend prefers for its output (no dot).
    // Default is "mp4" for broad compatibility; macOS HEVC-with-alpha
    // requires .mov and overrides this.
    virtual QString preferredExtension() const { return QStringLiteral("mp4"); }

signals:
    void stateChanged(RecordingState state);
    void recordingStarted(const QString &outputPath);
    void recordingStopped(const QString &outputPath);
    void recordingError(const QString &error);
};

} // namespace screencopy
