#include "MacCaptureBackend.h"
#include "MacCaptureBridge.h"
#include "MacPlatform.h"
#include "../platform/MacScreenCaptureBridge.h"

#include <QFile>
#include <QMetaObject>
#include <QString>

#include <CoreGraphics/CoreGraphics.h>

namespace screencopy {

MacCaptureBackend::MacCaptureBackend(PlatformIntegration *platform, QObject *parent)
    : CaptureBackend(parent)
    , m_platform(platform)
{
    // Register the app in System Settings > Privacy & Security > Screen Recording.
    // No-op if already granted.
    CGRequestScreenCaptureAccess();

    m_recorder = sc_recorder_create(this,
                                    &MacCaptureBackend::onStarted,
                                    &MacCaptureBackend::onStopped,
                                    &MacCaptureBackend::onError);
}

MacCaptureBackend::~MacCaptureBackend()
{
    if (m_recorder) {
        sc_recorder_destroy(static_cast<SCRecorderHandle>(m_recorder));
        m_recorder = nullptr;
    }
}

bool MacCaptureBackend::isAvailable() const
{
    return m_recorder != nullptr;
}

void MacCaptureBackend::startRecording(const QString &outputPath, const RecordingOptions &options)
{
    if (m_state != RecordingState::Idle || !m_recorder)
        return;

    m_outputPath = outputPath;
    m_state = RecordingState::Starting;
    emit stateChanged(m_state);

    // Pass the filter stored during source selection (from the picker). If
    // nullptr, Swift will enumerate and pick the primary display.
    auto *macPlatform = qobject_cast<MacPlatform *>(m_platform);
    void *filterHandle = macPlatform ? macPlatform->takeLastFilter() : nullptr;

    SCRecordingOptionsC opts {};
    opts.frame_rate = options.frameRate;
    if (!options.captureRegion.isNull()) {
        opts.has_region = true;
        opts.region_x = options.captureRegion.x();
        opts.region_y = options.captureRegion.y();
        opts.region_w = options.captureRegion.width();
        opts.region_h = options.captureRegion.height();
    }

    QByteArray pathBytes = outputPath.toUtf8();
    bool started = sc_recorder_start(static_cast<SCRecorderHandle>(m_recorder),
                                     pathBytes.constData(),
                                     filterHandle,
                                     opts);
    if (!started) {
        m_state = RecordingState::Idle;
        emit stateChanged(m_state);
        emit recordingError(QStringLiteral("Recording requires macOS 15.0 or later"));
    }
}

void MacCaptureBackend::stopRecording()
{
    if (m_state != RecordingState::Recording && m_state != RecordingState::Paused)
        return;

    m_state = RecordingState::Stopping;
    emit stateChanged(m_state);

    if (m_recorder)
        sc_recorder_stop(static_cast<SCRecorderHandle>(m_recorder));
}

void MacCaptureBackend::pauseRecording()
{
    // SCStream has no native pause — keep the stream running and just mark UI state.
    if (m_state != RecordingState::Recording)
        return;
    m_state = RecordingState::Paused;
    emit stateChanged(m_state);
}

void MacCaptureBackend::resumeRecording()
{
    if (m_state != RecordingState::Paused)
        return;
    m_state = RecordingState::Recording;
    emit stateChanged(m_state);
}

void MacCaptureBackend::cancelRecording()
{
    if (!m_recorder)
        return;

    sc_recorder_cancel(static_cast<SCRecorderHandle>(m_recorder));
    QString path = m_outputPath;
    m_outputPath.clear();
    m_state = RecordingState::Idle;
    emit stateChanged(m_state);

    if (!path.isEmpty())
        QFile::remove(path);
}

// ---------- Static C trampolines (called from Swift on SCK queue) ----------

void MacCaptureBackend::onStarted(void *ctx, const char *outputPath)
{
    auto *self = static_cast<MacCaptureBackend *>(ctx);
    if (!self)
        return;
    QString path = QString::fromUtf8(outputPath);
    QMetaObject::invokeMethod(self, [self, path]() {
        self->m_state = RecordingState::Recording;
        emit self->stateChanged(self->m_state);
        emit self->recordingStarted(path);
    }, Qt::QueuedConnection);
}

void MacCaptureBackend::onStopped(void *ctx, const char *outputPath)
{
    auto *self = static_cast<MacCaptureBackend *>(ctx);
    if (!self)
        return;
    QString path = QString::fromUtf8(outputPath);
    QMetaObject::invokeMethod(self, [self, path]() {
        self->m_outputPath.clear();
        self->m_state = RecordingState::Idle;
        emit self->stateChanged(self->m_state);
        emit self->recordingStopped(path);
    }, Qt::QueuedConnection);
}

void MacCaptureBackend::onError(void *ctx, const char *message)
{
    auto *self = static_cast<MacCaptureBackend *>(ctx);
    if (!self)
        return;
    QString msg = QString::fromUtf8(message);
    QMetaObject::invokeMethod(self, [self, msg]() {
        self->m_state = RecordingState::Idle;
        emit self->stateChanged(self->m_state);
        emit self->recordingError(msg);
    }, Qt::QueuedConnection);
}

} // namespace screencopy
