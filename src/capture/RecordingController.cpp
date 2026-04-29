#include "RecordingController.h"
#include "CaptureBackend.h"
#include "CursorTelemetry.h"
#include "PlatformIntegration.h"
#include <QDateTime>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>

namespace screencopy {

RecordingController::RecordingController(QObject *parent)
    : QObject(parent)
{
    m_elapsedTimer.setInterval(250);
    connect(&m_elapsedTimer, &QTimer::timeout, this, &RecordingController::updateElapsed);
}

void RecordingController::setBackend(CaptureBackend *backend)
{
    m_backend = backend;
    if (!m_backend)
        return;

    connect(m_backend, &CaptureBackend::recordingStarted, this, [this](const QString &) {
        m_recording = true;
        m_paused = false;
        m_elapsedSeconds = 0;
        m_accumulatedMs = 0;
        m_elapsedClock.start();
        m_elapsedTimer.start();

        if (m_cursorTelemetry)
            m_cursorTelemetry->startRecording();

        emit recordingChanged();
        emit pausedChanged();
        emit elapsedSecondsChanged();
    });

    connect(m_backend, &CaptureBackend::recordingStopped, this, [this](const QString &) {
        m_elapsedTimer.stop();

        if (m_cursorTelemetry)
            m_cursorTelemetry->stopRecording();

        // Use the controller's stored path — it's the source of truth.
        // Some backends emit an empty path on the signal.
        QString videoPath = m_outputPath;
        saveSessionFiles(videoPath);

        m_recording = false;
        m_paused = false;
        emit recordingChanged();
        emit pausedChanged();
        emit recordingFinished(videoPath);
        emit switchToEditor();
    });

    connect(m_backend, &CaptureBackend::recordingError, this, [this](const QString &err) {
        qWarning() << "Recording error:" << err;
        m_elapsedTimer.stop();

        if (m_cursorTelemetry) {
            m_cursorTelemetry->stopRecording();
            m_cursorTelemetry->clear();
        }

        m_recording = false;
        m_paused = false;
        emit recordingChanged();
        emit pausedChanged();
    });
}

void RecordingController::setPlatform(PlatformIntegration *platform)
{
    m_platform = platform;
    if (m_platform) {
        connect(m_platform, &PlatformIntegration::sourceSelected, this,
                [this](const QString &sourceId, const QString &sourceName) {
            m_selectedSource = sourceName;
            m_selectedSourceName = sourceId;
            emit selectedSourceChanged();

            if (m_autoRecord) {
                m_autoRecord = false;
                startRecording();
            }
        });

        connect(m_platform, &PlatformIntegration::sourceSelectionError, this,
                [](const QString &msg) {
            qWarning() << "Source selection error:" << msg;
        });
    }
}

void RecordingController::setCursorTelemetry(CursorTelemetry *telemetry)
{
    m_cursorTelemetry = telemetry;
}

QString RecordingController::elapsedFormatted() const
{
    int mins = m_elapsedSeconds / 60;
    int secs = m_elapsedSeconds % 60;
    return QString("%1:%2").arg(mins, 2, 10, QChar('0')).arg(secs, 2, 10, QChar('0'));
}

void RecordingController::setMicrophoneEnabled(bool enabled)
{
    if (m_microphoneEnabled != enabled) {
        m_microphoneEnabled = enabled;
        emit microphoneEnabledChanged();
    }
}

void RecordingController::setSystemAudioEnabled(bool enabled)
{
    if (m_systemAudioEnabled != enabled) {
        m_systemAudioEnabled = enabled;
        emit systemAudioEnabledChanged();
    }
}

void RecordingController::setWebcamEnabled(bool enabled)
{
    if (m_webcamEnabled != enabled) {
        m_webcamEnabled = enabled;
        emit webcamEnabledChanged();
    }
}

bool RecordingController::canRecord() const
{
    if (!m_backend || !m_backend->isAvailable())
        return false;
    return !m_backend->requiresSourceSelection() || hasSelectedSource();
}

void RecordingController::startRecording()
{
    if (m_recording || !m_backend)
        return;

    if (m_backend->state() != RecordingState::Idle)
        return;

    auto dir = m_platform ? m_platform->recordingsDir() : "/tmp";
    QDir().mkpath(dir);
    auto timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss");
    QString ext = m_backend->preferredExtension();
    m_outputPath = QString("%1/recording-%2.%3").arg(dir, timestamp, ext);

    RecordingOptions options;
    options.systemAudio = m_systemAudioEnabled;
    options.microphone = m_microphoneEnabled;
    options.frameRate = 60;

    if (!m_selectedSourceName.isEmpty())
        options.windowMode = m_selectedSourceName;
    else
        options.windowMode = "portal";

    if (m_captureMode == Area && !m_areaRect.isNull())
        options.captureRegion = m_areaRect;

    m_backend->startRecording(m_outputPath, options);
}

void RecordingController::stopRecording()
{
    if (!m_backend)
        return;

    if (!m_recording && m_backend->state() == RecordingState::Idle)
        return;

    m_backend->stopRecording();
}

void RecordingController::pauseRecording()
{
    if (!m_recording || m_paused || !m_backend)
        return;

    m_backend->pauseRecording();
    m_accumulatedMs += m_elapsedClock.elapsed();
    m_paused = true;
    emit pausedChanged();
}

void RecordingController::resumeRecording()
{
    if (!m_recording || !m_paused || !m_backend)
        return;

    m_backend->resumeRecording();
    m_elapsedClock.restart();
    m_paused = false;
    emit pausedChanged();
}

void RecordingController::togglePause()
{
    if (m_paused)
        resumeRecording();
    else
        pauseRecording();
}

void RecordingController::restartRecording()
{
    if (!m_recording)
        return;
    cancelRecording();
    startRecording();
}

void RecordingController::cancelRecording()
{
    if (!m_backend)
        return;

    if (!m_recording && m_backend->state() == RecordingState::Idle)
        return;

    m_backend->cancelRecording();
    m_recording = false;
    m_paused = false;
    m_elapsedSeconds = 0;
    m_accumulatedMs = 0;
    m_elapsedTimer.stop();

    if (m_cursorTelemetry) {
        m_cursorTelemetry->stopRecording();
        m_cursorTelemetry->clear();
    }

    emit recordingChanged();
    emit pausedChanged();
    emit elapsedSecondsChanged();
}

void RecordingController::selectSource(const QString &sourceId, const QString &sourceName)
{
    m_selectedSource = sourceName;
    m_selectedSourceName = sourceId;
    emit selectedSourceChanged();
}

void RecordingController::openSourceSelector()
{
    if (m_platform)
        m_platform->requestSourceSelection();
}

void RecordingController::selectFullScreen()
{
    m_captureMode = FullScreen;
    m_autoRecord = true;
    emit captureModeChanged();

    if (m_platform)
        m_platform->selectFullScreen();
}

void RecordingController::selectWindow()
{
    m_captureMode = Window;
    m_autoRecord = true;
    emit captureModeChanged();

    if (m_platform)
        m_platform->selectWindow();
}

void RecordingController::beginAreaSelection()
{
    m_captureMode = Area;
    emit captureModeChanged();
    emit areaSelectionRequested();
}

void RecordingController::selectArea(int x, int y, int w, int h)
{
    m_areaRect = QRect(x, y, w, h);
    m_selectedSourceName = QString("area:%1,%2,%3x%4").arg(x).arg(y).arg(w).arg(h);
    m_selectedSource = QString("Area (%1×%2)").arg(w).arg(h);
    emit selectedSourceChanged();
    startRecording();
}

void RecordingController::saveSessionFiles(const QString &videoPath)
{
    qint64 createdAt = QDateTime::currentMSecsSinceEpoch();

    if (m_cursorTelemetry && !m_cursorTelemetry->samples().isEmpty()) {
        QString telemetryPath = videoPath + ".cursor.json";
        m_cursorTelemetry->saveToFile(telemetryPath);
    }

    QJsonObject sessionObj;
    sessionObj["screenVideoPath"] = videoPath;
    sessionObj["createdAt"] = createdAt;

    QFileInfo fi(videoPath);
    QString manifestPath = fi.absolutePath() + "/" + fi.completeBaseName() + ".session.json";

    QFile manifestFile(manifestPath);
    if (manifestFile.open(QIODevice::WriteOnly)) {
        manifestFile.write(QJsonDocument(sessionObj).toJson(QJsonDocument::Indented));
        manifestFile.close();
    }
}

void RecordingController::updateElapsed()
{
    if (!m_recording)
        return;

    qint64 totalMs = m_accumulatedMs;
    if (!m_paused)
        totalMs += m_elapsedClock.elapsed();

    int newSeconds = static_cast<int>(totalMs / 1000);
    if (newSeconds != m_elapsedSeconds) {
        m_elapsedSeconds = newSeconds;
        emit elapsedSecondsChanged();
    }
}

} // namespace screencopy
