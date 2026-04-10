#include "GpuScreenRecorder.h"
#include "PlatformIntegration.h"
#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include <csignal>

namespace screencopy {

static const QString GPU_RECORDER_PATH = "/usr/bin/gpu-screen-recorder";
static const int POLL_INTERVAL_MS = 200;
static const int POLL_TIMEOUT_MS = 30000;
static const int STOP_TIMEOUT_MS = 5000;

GpuScreenRecorder::GpuScreenRecorder(PlatformIntegration *platform, QObject *parent)
    : CaptureBackend(parent)
    , m_platform(platform)
{
    m_startTimeout.setSingleShot(true);
    m_startTimeout.setInterval(POLL_TIMEOUT_MS);

    connect(&m_startTimeout, &QTimer::timeout, this, [this]() {
        if (m_state == RecordingState::Starting) {
            qWarning() << "gpu-screen-recorder: timed out waiting for portal selection";
            emit recordingError("Timed out waiting for screen selection");
            cancelRecording();
        }
    });
}

GpuScreenRecorder::~GpuScreenRecorder()
{
    killIfRunning();
}

bool GpuScreenRecorder::isAvailable() const
{
    return QFile::exists(GPU_RECORDER_PATH);
}

void GpuScreenRecorder::startRecording(const QString &outputPath)
{
    startRecording(outputPath, RecordingOptions{});
}

void GpuScreenRecorder::startRecording(const QString &outputPath, const RecordingOptions &options)
{
    if (m_state != RecordingState::Idle)
        return;

    m_outputPath = outputPath;
    m_state = RecordingState::Starting;
    emit stateChanged(m_state);

    m_process = new QProcess(this);

    QStringList args = {
        "-w", options.windowMode,
        "-k", "auto",
        "-s", "0x0",
        "-f", QString::number(options.frameRate),
        "-fm", "cfr",
        "-fallback-cpu-encoding", "yes",
        "-o", outputPath,
    };

    // Audio sources
    QStringList audioSources;
    if (options.systemAudio)
        audioSources << "default_output";
    if (options.microphone)
        audioSources << "default_input";

    if (!audioSources.isEmpty()) {
        args << "-a" << audioSources.join("|") << "-ac" << "aac";
    }

    qDebug() << "gpu-screen-recorder:" << GPU_RECORDER_PATH << args.join(" ");

    connect(m_process, &QProcess::finished, this, [this](int exitCode, QProcess::ExitStatus status) {
        Q_UNUSED(exitCode);
        Q_UNUSED(status);
        m_startTimeout.stop();
        if (m_pollTimer) {
            m_pollTimer->stop();
            m_pollTimer->deleteLater();
            m_pollTimer = nullptr;
        }
        if (m_state == RecordingState::Stopping) {
            m_state = RecordingState::Idle;
            emit stateChanged(m_state);
            emit recordingStopped(m_outputPath);
        } else if (m_state != RecordingState::Idle) {
            // Unexpected exit
            qWarning() << "gpu-screen-recorder: unexpected exit, code" << exitCode;
            m_state = RecordingState::Idle;
            emit stateChanged(m_state);
            emit recordingError(QString("Process exited unexpectedly (code %1)").arg(exitCode));
        }
        m_process->deleteLater();
        m_process = nullptr;
    });

    connect(m_process, &QProcess::readyReadStandardError, this, [this]() {
        auto data = m_process->readAllStandardError();
        qDebug() << "gpu-screen-recorder stderr:" << data;
    });

    m_process->start(GPU_RECORDER_PATH, args);
    m_startTimeout.start();

    // Poll for output file creation — indicates portal selection done
    m_pollTimer = new QTimer(this);
    m_pollTimer->setInterval(POLL_INTERVAL_MS);
    connect(m_pollTimer, &QTimer::timeout, this, [this]() {
        if (m_state != RecordingState::Starting) {
            m_pollTimer->stop();
            m_pollTimer->deleteLater();
            m_pollTimer = nullptr;
            return;
        }
        if (QFileInfo::exists(m_outputPath)) {
            m_startTimeout.stop();
            m_pollTimer->stop();
            m_pollTimer->deleteLater();
            m_pollTimer = nullptr;
            m_state = RecordingState::Recording;
            emit stateChanged(m_state);
            emit recordingStarted(m_outputPath);
            qDebug() << "gpu-screen-recorder: recording started ->" << m_outputPath;
        }
    });
    m_pollTimer->start();
}

void GpuScreenRecorder::stopRecording()
{
    if (m_state != RecordingState::Recording && m_state != RecordingState::Paused)
        return;

    m_state = RecordingState::Stopping;
    emit stateChanged(m_state);

    if (m_process) {
        // Graceful: SIGINT, then force SIGKILL after timeout
        m_process->terminate();

        QTimer::singleShot(STOP_TIMEOUT_MS, this, [this]() {
            if (m_process && m_state == RecordingState::Stopping) {
                qWarning() << "gpu-screen-recorder: force killing after timeout";
                m_process->kill();
            }
        });
    }
}

void GpuScreenRecorder::pauseRecording()
{
    if (m_state != RecordingState::Recording || !m_process)
        return;

    ::kill(m_process->processId(), SIGUSR2);
    m_state = RecordingState::Paused;
    emit stateChanged(m_state);
}

void GpuScreenRecorder::resumeRecording()
{
    if (m_state != RecordingState::Paused || !m_process)
        return;

    ::kill(m_process->processId(), SIGUSR2);
    m_state = RecordingState::Recording;
    emit stateChanged(m_state);
}

void GpuScreenRecorder::cancelRecording()
{
    killIfRunning();

    if (!m_outputPath.isEmpty())
        QFile::remove(m_outputPath);

    m_state = RecordingState::Idle;
    emit stateChanged(m_state);
}

void GpuScreenRecorder::killIfRunning()
{
    m_startTimeout.stop();
    if (m_pollTimer) {
        m_pollTimer->stop();
        m_pollTimer->deleteLater();
        m_pollTimer = nullptr;
    }
    if (m_process) {
        m_process->disconnect();
        m_process->terminate();
        if (!m_process->waitForFinished(STOP_TIMEOUT_MS))
            m_process->kill();
        m_process->deleteLater();
        m_process = nullptr;
    }
}

} // namespace screencopy
