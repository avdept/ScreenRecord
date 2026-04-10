#include "RecordingController.h"
#include "GpuScreenRecorder.h"
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

void RecordingController::setGpuRecorder(GpuScreenRecorder *recorder)
{
    m_gpuRecorder = recorder;
    if (!m_gpuRecorder)
        return;

    // When recording actually starts (portal selected, file created)
    connect(m_gpuRecorder, &GpuScreenRecorder::recordingStarted, this, [this](const QString &path) {
        qDebug() << "Recording started:" << path;

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

    // When recording stops and file is finalized
    connect(m_gpuRecorder, &GpuScreenRecorder::recordingStopped, this, [this](const QString &path) {
        m_elapsedTimer.stop();

        // Stop cursor telemetry
        if (m_cursorTelemetry)
            m_cursorTelemetry->stopRecording();

        // Save session files alongside the video
        saveSessionFiles(path);

        m_recording = false;
        m_paused = false;
        emit recordingChanged();
        emit pausedChanged();
        emit recordingFinished(path);
        emit switchToEditor();
    });

    connect(m_gpuRecorder, &GpuScreenRecorder::recordingError, this, [this](const QString &err) {
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
            qDebug() << "Source selected:" << sourceName << "(" << sourceId << ")";
            m_selectedSource = sourceName;
            m_selectedSourceName = sourceId;
            emit selectedSourceChanged();
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
    // On Wayland with gpu-screen-recorder, always allow — portal handles source selection
    if (m_gpuRecorder && m_gpuRecorder->isAvailable())
        return true;
    // On other platforms, need a selected source
    return hasSelectedSource();
}

void RecordingController::startRecording()
{
    if (m_recording)
        return;

    // Don't start again if gpu-screen-recorder is already spawned (portal open)
    if (m_gpuRecorder && m_gpuRecorder->state() != RecordingState::Idle)
        return;

    if (!m_gpuRecorder || !m_gpuRecorder->isAvailable()) {
        // TODO: non-gpu-screen-recorder path
        qWarning() << "No recording backend available";
        return;
    }

    // Build output path
    auto dir = m_platform ? m_platform->recordingsDir() : "/tmp";
    QDir().mkpath(dir);
    auto timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss");
    auto outputPath = QString("%1/recording-%2.mp4").arg(dir, timestamp);

    // Configure recording options from current toggle state
    RecordingOptions options;
    options.systemAudio = m_systemAudioEnabled;
    options.microphone = m_microphoneEnabled;
    options.frameRate = 60;
    options.windowMode = "portal";  // gpu-screen-recorder shows portal picker itself

    m_gpuRecorder->startRecording(outputPath, options);
    // Don't set m_recording=true yet — wait for recordingStarted signal
    // (fires when portal selection is done and recording actually begins)
}

void RecordingController::stopRecording()
{
    if (!m_gpuRecorder)
        return;

    // Allow stopping even during Starting state (portal selection)
    if (!m_recording && m_gpuRecorder->state() == RecordingState::Idle)
        return;

    m_gpuRecorder->stopRecording();
    // Finalization happens in the recordingStopped signal handler
}

void RecordingController::pauseRecording()
{
    if (!m_recording || m_paused || !m_gpuRecorder)
        return;

    m_gpuRecorder->pauseRecording();
    m_accumulatedMs += m_elapsedClock.elapsed();
    m_paused = true;
    emit pausedChanged();
}

void RecordingController::resumeRecording()
{
    if (!m_recording || !m_paused || !m_gpuRecorder)
        return;

    m_gpuRecorder->resumeRecording();
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
    if (!m_gpuRecorder)
        return;

    if (!m_recording && m_gpuRecorder->state() == RecordingState::Idle)
        return;

    m_gpuRecorder->cancelRecording();
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
    // On Wayland with gpu-screen-recorder, "select source" means "start recording"
    // because gpu-screen-recorder shows the portal picker itself at recording start
    if (m_gpuRecorder && m_gpuRecorder->isAvailable() && !m_recording) {
        startRecording();
        return;
    }

    // On other platforms, show the native source picker
    if (m_platform)
        m_platform->requestSourceSelection();
}

void RecordingController::saveSessionFiles(const QString &videoPath)
{
    qint64 createdAt = QDateTime::currentMSecsSinceEpoch();

    // 1. Save cursor telemetry
    if (m_cursorTelemetry && !m_cursorTelemetry->samples().isEmpty()) {
        QString telemetryPath = videoPath + ".cursor.json";
        m_cursorTelemetry->saveToFile(telemetryPath);
        qDebug() << "Saved cursor telemetry:" << telemetryPath
                 << "(" << m_cursorTelemetry->samples().size() << "samples)";
    }

    // 2. Save session manifest
    QJsonObject sessionObj;
    sessionObj["screenVideoPath"] = videoPath;
    sessionObj["createdAt"] = createdAt;

    // TODO: webcam video path when webcam recording is implemented

    QFileInfo fi(videoPath);
    QString manifestPath = fi.absolutePath() + "/" + fi.completeBaseName() + ".session.json";

    QFile manifestFile(manifestPath);
    if (manifestFile.open(QIODevice::WriteOnly)) {
        manifestFile.write(QJsonDocument(sessionObj).toJson(QJsonDocument::Indented));
        manifestFile.close();
        qDebug() << "Saved session manifest:" << manifestPath;
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
