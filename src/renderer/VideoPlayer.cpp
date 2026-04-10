#include "VideoPlayer.h"
#include "FFmpegDecoder.h"
#include <QPainter>

namespace screencopy {

// ── DecodeWorker ────────────────────────────────────────────

DecodeWorker::DecodeWorker(QObject *parent)
    : QObject(parent)
{
}

DecodeWorker::~DecodeWorker()
{
    stop();
}

void DecodeWorker::setDecoder(FFmpegDecoder *decoder)
{
    m_decoder = decoder;
}

void DecodeWorker::setCutRegions(const QList<CutRegion> &regions)
{
    QMutexLocker lock(&m_mutex);
    m_cutRegions = regions;
}

void DecodeWorker::startFrom(qint64 posMs)
{
    stop();

    m_stopRequested.store(false, std::memory_order_release);
    m_running.store(true, std::memory_order_release);

    {
        QMutexLocker lock(&m_mutex);
        m_queue.clear();
    }

    m_thread = QThread::create([this, posMs]() { decodeLoop(posMs); });
    m_thread->start();
}

void DecodeWorker::stop()
{
    m_stopRequested.store(true, std::memory_order_release);
    m_spaceAvailable.wakeAll();

    if (m_thread) {
        m_thread->wait();
        delete m_thread;
        m_thread = nullptr;
    }

    m_running.store(false, std::memory_order_release);
}

DecodedFrame DecodeWorker::takeFrame()
{
    QMutexLocker lock(&m_mutex);
    if (m_queue.isEmpty())
        return {};

    auto frame = m_queue.dequeue();
    m_spaceAvailable.wakeOne(); // wake decode thread if it was waiting for space
    return frame;
}

int DecodeWorker::buffered() const
{
    QMutexLocker lock(&m_mutex);
    return m_queue.size();
}

bool DecodeWorker::isInCutRegion(qint64 ms, qint64 *skipToMs) const
{
    // m_mutex must be held by caller
    for (const auto &cut : m_cutRegions) {
        if (ms >= cut.startMs && ms < cut.endMs) {
            *skipToMs = cut.endMs;
            return true;
        }
    }
    return false;
}

void DecodeWorker::decodeLoop(qint64 startMs)
{
    if (!m_decoder) return;

    m_decoder->seekTo(startMs);

    while (!m_stopRequested.load(std::memory_order_acquire)) {
        // Wait if queue is full
        {
            QMutexLocker lock(&m_mutex);
            while (m_queue.size() >= MAX_QUEUE && !m_stopRequested.load(std::memory_order_acquire)) {
                m_spaceAvailable.wait(&m_mutex, 5);
            }
        }

        if (m_stopRequested.load(std::memory_order_acquire)) break;

        // Decode next raw frame (no sws_scale — very fast)
        AVFrame *rawFrame = m_decoder->decodeNextRawFrame();
        if (!rawFrame) break; // EOF

        qint64 ptsMs = m_decoder->lastFramePtsMs();

        // Check if this frame is in a cut region OR approaching one
        qint64 skipTo = 0;
        bool inCut = false;
        bool nearCut = false;
        {
            QMutexLocker lock(&m_mutex);
            inCut = isInCutRegion(ptsMs, &skipTo);
            if (!inCut) {
                // Check if we're approaching a cut region within 100ms
                // (don't enqueue these last frames — seek ahead early)
                for (const auto &cut : m_cutRegions) {
                    if (ptsMs < cut.startMs && cut.startMs - ptsMs < 100) {
                        nearCut = true;
                        skipTo = cut.endMs;
                        break;
                    }
                }
            }
        }

        if (inCut || nearCut) {
            // Enqueue the current frame if it's before the cut (not in it)
            if (nearCut && rawFrame) {
                DecodedFrame df;
                df.rawFrame = rawFrame;
                df.ptsMs = ptsMs;
                QMutexLocker lock(&m_mutex);
                m_queue.enqueue(df);
                rawFrame = nullptr;
            } else {
                av_frame_free(&rawFrame);
            }

            // Skip through the cut — no seek/flush, just decode forward
            // discarding frames. Keeps HW decoder warm. The main thread
            // displays buffered frames during this time.
            QImage skipFrame = m_decoder->skipTo(skipTo);

            if (!skipFrame.isNull()) {
                DecodedFrame df;
                df.image = skipFrame;
                df.ptsMs = m_decoder->lastFramePtsMs();
                QMutexLocker lock(&m_mutex);
                m_queue.enqueue(df);
            }
            continue;
        }

        // Enqueue raw frame for the main thread (no conversion yet — fast)
        {
            DecodedFrame df;
            df.rawFrame = rawFrame;
            df.ptsMs = ptsMs;
            QMutexLocker lock(&m_mutex);
            m_queue.enqueue(df);
        }
    }

    m_running.store(false, std::memory_order_release);
}

// ── VideoPlayer ─────────────────────────────────────────────

VideoPlayer::VideoPlayer(QQuickItem *parent)
    : QQuickPaintedItem(parent)
    , m_decoder(std::make_unique<FFmpegDecoder>())
    , m_scrubDecoder(std::make_unique<FFmpegDecoder>())
{
    setRenderTarget(QQuickPaintedItem::FramebufferObject);
    m_worker.setDecoder(m_decoder.get());

    connect(&m_frameTimer, &QTimer::timeout, this, &VideoPlayer::advanceFrame);
    connect(this, &QQuickItem::widthChanged, this, [this]() { m_targetRect = QRectF(); });
    connect(this, &QQuickItem::heightChanged, this, [this]() { m_targetRect = QRectF(); });
}

VideoPlayer::~VideoPlayer()
{
    m_worker.stop();
}

void VideoPlayer::setSource(const QString &path)
{
    if (m_source == path) return;
    setPlaying(false);

    m_source = path;
    m_filePath = path;
    if (m_filePath.startsWith("file://"))
        m_filePath = QUrl(m_filePath).toLocalFile();

    emit sourceChanged();

    m_decoder->close();
    m_scrubDecoder->close();
    m_loaded = false;
    m_positionMs = 0;
    m_durationMs = 0;
    m_currentFrame = QImage();
    m_targetRect = QRectF();

    if (!m_decoder->open(m_filePath)) {
        emit loadedChanged();
        return;
    }

    // Open a separate decoder for scrubbing (used only when paused)
    m_scrubDecoder->open(m_filePath);

    m_durationMs = m_decoder->durationMs();
    m_frameRate = m_decoder->frameRate();
    if (m_frameRate <= 0) m_frameRate = 30.0;
    m_videoWidth = m_decoder->width();
    m_videoHeight = m_decoder->height();
    m_loaded = true;

    emit durationChanged();
    emit frameRateChanged();
    emit videoSizeChanged();
    emit loadedChanged();

    // Show first frame via scrub decoder
    QImage frame = m_scrubDecoder->decodeNextFrame();
    if (!frame.isNull()) {
        m_currentFrame = frame;
        update();
    }
    emit positionChanged();
}

void VideoPlayer::setPlaying(bool playing)
{
    if (m_playing == playing) return;
    m_playing = playing;

    if (m_playing && m_loaded) {
        // Start background decode thread from current position
        m_worker.setCutRegions(m_cutRegions);
        m_worker.startFrom(m_positionMs);

        // Wait briefly for buffer to fill before starting display
        // (non-blocking spin — just lets the decode thread get ahead)
        QElapsedTimer fillWait;
        fillWait.start();
        while (m_worker.buffered() < 5 && fillWait.elapsed() < 100) {
            QThread::usleep(500);
        }

        int intervalMs = qMax(1, qRound(1000.0 / m_frameRate));
        m_frameTimer.start(intervalMs);
    } else {
        m_frameTimer.stop();
        m_worker.stop();
    }

    emit playingChanged();
}

void VideoPlayer::setPosition(qint64 ms)
{
    if (!m_loaded) return;
    ms = qBound(qint64(0), ms, m_durationMs);
    if (m_positionMs == ms) return;

    m_positionMs = ms;

    if (m_playing) {
        // Restart decode thread from new position
        m_worker.setCutRegions(m_cutRegions);
        m_worker.startFrom(ms);
    } else {
        // Scrub: use the separate scrub decoder (main thread, throttled)
        if (m_seekThrottle.isValid() && m_seekThrottle.elapsed() < 66) {
            emit positionChanged();
            return;
        }
        m_seekThrottle.start();

        qint64 lastPts = m_scrubDecoder->lastFramePtsMs();
        qint64 delta = ms - lastPts;

        QImage frame;
        if (delta > 0 && delta < 3000) {
            // Forward scrub within 3s — skip forward without flush (fast)
            frame = m_scrubDecoder->skipTo(ms);
        } else {
            // Backward or large jump — must seek
            frame = m_scrubDecoder->decodeFrameAt(ms);
        }

        if (!frame.isNull()) {
            m_currentFrame = frame;
            m_targetRect = QRectF();
            update();
        }
    }

    emit positionChanged();
}

void VideoPlayer::setSegments(const QVariantList &segments)
{
    m_cutRegions.clear();

    struct Seg { qint64 s, e; };
    QList<Seg> keepSegs;
    for (const auto &v : segments) {
        auto map = v.toMap();
        keepSegs.append({map["startMs"].toLongLong(), map["endMs"].toLongLong()});
    }
    std::sort(keepSegs.begin(), keepSegs.end(), [](const auto &a, const auto &b) { return a.s < b.s; });

    for (int i = 0; i < keepSegs.size() - 1; i++) {
        qint64 gapStart = keepSegs[i].e;
        qint64 gapEnd = keepSegs[i + 1].s;
        if (gapEnd > gapStart)
            m_cutRegions.append({gapStart, gapEnd});
    }

    // Update worker if playing
    m_worker.setCutRegions(m_cutRegions);
}

void VideoPlayer::play()
{
    if (!m_loaded) return;
    if (m_positionMs >= m_durationMs) setPosition(0);
    setPlaying(true);
}

void VideoPlayer::pause() { setPlaying(false); }
void VideoPlayer::togglePlayPause() { if (m_playing) pause(); else play(); }
void VideoPlayer::seek(qint64 ms) { setPosition(ms); }

void VideoPlayer::stepForward()
{
    if (!m_loaded) return;
    setPosition(qMin(m_positionMs + qRound(1000.0 / m_frameRate), m_durationMs));
}

void VideoPlayer::stepBackward()
{
    if (!m_loaded) return;
    setPosition(qMax(m_positionMs - qRound(1000.0 / m_frameRate), qint64(0)));
}

void VideoPlayer::advanceFrame()
{
    if (!m_playing || !m_loaded) return;

    DecodedFrame frame = m_worker.takeFrame();

    if (!frame.rawFrame && frame.image.isNull()) {
        if (!m_worker.isRunning() && m_worker.buffered() == 0) {
            m_positionMs = m_durationMs;
            setPlaying(false);
            emit positionChanged();
            emit ended();
        }
        return;
    }

    // Convert raw AVFrame to QImage (one frame, ~3-5ms)
    if (frame.rawFrame) {
        m_currentFrame = m_scrubDecoder->frameToImage(frame.rawFrame);
        av_frame_free(&frame.rawFrame);
    } else {
        m_currentFrame = frame.image;
    }

    m_positionMs = frame.ptsMs;
    update();

    // Throttle position updates to ~15fps
    if (!m_seekThrottle.isValid() || m_seekThrottle.elapsed() > 66) {
        m_seekThrottle.start();
        emit positionChanged();
    }
}

void VideoPlayer::paint(QPainter *painter)
{
    if (m_currentFrame.isNull()) return;
    painter->setRenderHint(QPainter::SmoothPixmapTransform);

    if (m_targetRect.isEmpty()) {
        QSizeF itemSize(width(), height());
        QSizeF frameSize(m_currentFrame.size());
        QSizeF scaled = frameSize.scaled(itemSize, Qt::KeepAspectRatio);
        m_targetRect = QRectF(
            (itemSize.width() - scaled.width()) / 2.0,
            (itemSize.height() - scaled.height()) / 2.0,
            scaled.width(), scaled.height()
        );
    }
    painter->drawImage(m_targetRect, m_currentFrame);
}

} // namespace screencopy
