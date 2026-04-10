#pragma once

#include <QObject>

namespace screencopy {

class PlaybackController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(qint64 currentTimeMs READ currentTimeMs WRITE setCurrentTimeMs NOTIFY currentTimeMsChanged)
    Q_PROPERTY(qint64 durationMs READ durationMs WRITE setDurationMs NOTIFY durationMsChanged)
    Q_PROPERTY(bool playing READ isPlaying NOTIFY playingChanged)
    Q_PROPERTY(double playbackSpeed READ playbackSpeed WRITE setPlaybackSpeed NOTIFY playbackSpeedChanged)

public:
    explicit PlaybackController(QObject *parent = nullptr);

    qint64 currentTimeMs() const { return m_currentTimeMs; }
    void setCurrentTimeMs(qint64 ms);

    qint64 durationMs() const { return m_durationMs; }
    void setDurationMs(qint64 ms);

    bool isPlaying() const { return m_playing; }
    double playbackSpeed() const { return m_playbackSpeed; }
    void setPlaybackSpeed(double speed);

    Q_INVOKABLE void play();
    Q_INVOKABLE void pause();
    Q_INVOKABLE void togglePlayPause();
    Q_INVOKABLE void seekTo(qint64 ms);

signals:
    void currentTimeMsChanged();
    void durationMsChanged();
    void playingChanged();
    void playbackSpeedChanged();

private:
    qint64 m_currentTimeMs = 0;
    qint64 m_durationMs = 0;
    bool m_playing = false;
    double m_playbackSpeed = 1.0;
};

} // namespace screencopy
