#pragma once

#include <QObject>
#include <QTimer>
#include <QElapsedTimer>

namespace screencopy {

class GpuScreenRecorder;
class PlatformIntegration;
class CursorTelemetry;

class RecordingController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool recording READ isRecording NOTIFY recordingChanged)
    Q_PROPERTY(bool paused READ isPaused NOTIFY pausedChanged)
    Q_PROPERTY(int elapsedSeconds READ elapsedSeconds NOTIFY elapsedSecondsChanged)
    Q_PROPERTY(QString elapsedFormatted READ elapsedFormatted NOTIFY elapsedSecondsChanged)
    Q_PROPERTY(bool microphoneEnabled READ microphoneEnabled WRITE setMicrophoneEnabled NOTIFY microphoneEnabledChanged)
    Q_PROPERTY(bool systemAudioEnabled READ systemAudioEnabled WRITE setSystemAudioEnabled NOTIFY systemAudioEnabledChanged)
    Q_PROPERTY(bool webcamEnabled READ webcamEnabled WRITE setWebcamEnabled NOTIFY webcamEnabledChanged)
    Q_PROPERTY(QString selectedSource READ selectedSource NOTIFY selectedSourceChanged)
    Q_PROPERTY(bool hasSelectedSource READ hasSelectedSource NOTIFY selectedSourceChanged)
    Q_PROPERTY(bool canRecord READ canRecord NOTIFY canRecordChanged)
    Q_PROPERTY(double audioLevel READ audioLevel NOTIFY audioLevelChanged)

public:
    explicit RecordingController(QObject *parent = nullptr);

    void setGpuRecorder(GpuScreenRecorder *recorder);
    void setPlatform(PlatformIntegration *platform);
    void setCursorTelemetry(CursorTelemetry *telemetry);

    bool isRecording() const { return m_recording; }
    bool isPaused() const { return m_paused; }
    int elapsedSeconds() const { return m_elapsedSeconds; }
    QString elapsedFormatted() const;

    bool microphoneEnabled() const { return m_microphoneEnabled; }
    void setMicrophoneEnabled(bool enabled);
    bool systemAudioEnabled() const { return m_systemAudioEnabled; }
    void setSystemAudioEnabled(bool enabled);
    bool webcamEnabled() const { return m_webcamEnabled; }
    void setWebcamEnabled(bool enabled);

    QString selectedSource() const { return m_selectedSource; }
    bool hasSelectedSource() const { return !m_selectedSource.isEmpty(); }
    bool canRecord() const;
    double audioLevel() const { return m_audioLevel; }

    Q_INVOKABLE void startRecording();
    Q_INVOKABLE void stopRecording();
    Q_INVOKABLE void pauseRecording();
    Q_INVOKABLE void resumeRecording();
    Q_INVOKABLE void togglePause();
    Q_INVOKABLE void restartRecording();
    Q_INVOKABLE void cancelRecording();
    Q_INVOKABLE void selectSource(const QString &sourceId, const QString &sourceName);
    Q_INVOKABLE void openSourceSelector();

signals:
    void recordingChanged();
    void pausedChanged();
    void elapsedSecondsChanged();
    void microphoneEnabledChanged();
    void systemAudioEnabledChanged();
    void webcamEnabledChanged();
    void selectedSourceChanged();
    void canRecordChanged();
    void audioLevelChanged();
    void recordingFinished(const QString &videoPath);
    void switchToEditor();

private:
    void updateElapsed();
    void saveSessionFiles(const QString &videoPath);

    GpuScreenRecorder *m_gpuRecorder = nullptr;
    PlatformIntegration *m_platform = nullptr;
    CursorTelemetry *m_cursorTelemetry = nullptr;

    bool m_recording = false;
    bool m_paused = false;
    int m_elapsedSeconds = 0;
    bool m_microphoneEnabled = false;
    bool m_systemAudioEnabled = true;
    bool m_webcamEnabled = false;
    QString m_selectedSource;
    QString m_selectedSourceName;
    double m_audioLevel = 0.0;

    QTimer m_elapsedTimer;
    QElapsedTimer m_elapsedClock;
    qint64 m_accumulatedMs = 0;
};

} // namespace screencopy
