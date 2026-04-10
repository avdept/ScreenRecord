#include "PlaybackController.h"
#include <QtMath>

namespace screencopy {

PlaybackController::PlaybackController(QObject *parent)
    : QObject(parent)
{
}

void PlaybackController::setCurrentTimeMs(qint64 ms)
{
    ms = qBound(qint64(0), ms, m_durationMs);
    if (m_currentTimeMs != ms) {
        m_currentTimeMs = ms;
        emit currentTimeMsChanged();
    }
}

void PlaybackController::setDurationMs(qint64 ms)
{
    if (m_durationMs != ms) {
        m_durationMs = ms;
        emit durationMsChanged();
    }
}

void PlaybackController::setPlaybackSpeed(double speed)
{
    speed = qBound(0.1, speed, 16.0);
    if (!qFuzzyCompare(m_playbackSpeed, speed)) {
        m_playbackSpeed = speed;
        emit playbackSpeedChanged();
    }
}

void PlaybackController::play()
{
    if (!m_playing) {
        m_playing = true;
        emit playingChanged();
    }
}

void PlaybackController::pause()
{
    if (m_playing) {
        m_playing = false;
        emit playingChanged();
    }
}

void PlaybackController::togglePlayPause()
{
    m_playing = !m_playing;
    emit playingChanged();
}

void PlaybackController::seekTo(qint64 ms)
{
    setCurrentTimeMs(ms);
}

} // namespace screencopy
