#pragma once

#include <QObject>
#include <QRect>
#include <QString>

namespace screencopy {

class PlatformIntegration;

enum class RecordingState {
    Idle,
    Starting,
    Recording,
    Paused,
    Stopping
};

struct RecordingOptions {
    bool systemAudio = true;
    bool microphone = false;
    int frameRate = 60;
    QString windowMode = "portal";  // "portal", "focused", or a window/display ID
    QRect captureRegion;            // non-null for area capture
};

class CaptureBackend : public QObject
{
    Q_OBJECT
    Q_PROPERTY(RecordingState state READ state NOTIFY stateChanged)

public:
    explicit CaptureBackend(QObject *parent = nullptr) : QObject(parent) {}

    // Compile-time platform selection — returns the backend for the current
    // platform, or nullptr where no implementation exists.
    static CaptureBackend *create(PlatformIntegration *platform, QObject *parent = nullptr);

    virtual RecordingState state() const = 0;

    virtual void startRecording(const QString &outputPath, const RecordingOptions &options) = 0;
    virtual void stopRecording() = 0;
    virtual void pauseRecording() = 0;
    virtual void resumeRecording() = 0;
    virtual void cancelRecording() = 0;
    virtual bool isAvailable() const = 0;

    // True when the backend cannot start without a user-selected source.
    // macOS picker workflow needs this; Linux gpu-screen-recorder handles
    // selection itself via the XDG portal at start time.
    virtual bool requiresSourceSelection() const { return true; }

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
