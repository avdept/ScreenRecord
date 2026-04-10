#pragma once

#include <QQuickPaintedItem>
#include <QImage>
#include <QTimer>
#include <QElapsedTimer>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QList>
#include <QQueue>
#include <memory>
#include <atomic>

extern "C" {
#include <libavutil/frame.h>
}

namespace screencopy {

class FFmpegDecoder;

struct CutRegion {
    qint64 startMs = 0;
    qint64 endMs = 0;
};

struct DecodedFrame {
    AVFrame *rawFrame = nullptr;
    QImage image;
    qint64 ptsMs = 0;
};

// Background decode thread — produces frames into a queue.
// Skips cut regions by decoding forward (no seek/flush).
class DecodeWorker : public QObject
{
    Q_OBJECT

public:
    explicit DecodeWorker(QObject *parent = nullptr);
    ~DecodeWorker() override;

    void setDecoder(FFmpegDecoder *decoder);
    void setCutRegions(const QList<CutRegion> &regions);

    void startFrom(qint64 posMs);
    void stop();

    DecodedFrame takeFrame();
    int buffered() const;
    bool isRunning() const { return m_running.load(std::memory_order_acquire); }

private:
    void decodeLoop(qint64 startMs);
    bool isInCutRegion(qint64 ms, qint64 *skipToMs) const;

    FFmpegDecoder *m_decoder = nullptr;
    QList<CutRegion> m_cutRegions;

    static constexpr int MAX_QUEUE = 180;
    QQueue<DecodedFrame> m_queue;
    mutable QMutex m_mutex;
    QWaitCondition m_spaceAvailable;

    QThread *m_thread = nullptr;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_stopRequested{false};
};

class VideoPlayer : public QQuickPaintedItem
{
    Q_OBJECT
    Q_PROPERTY(QString source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(bool playing READ isPlaying WRITE setPlaying NOTIFY playingChanged)
    Q_PROPERTY(qint64 position READ position WRITE setPosition NOTIFY positionChanged)
    Q_PROPERTY(qint64 duration READ duration NOTIFY durationChanged)
    Q_PROPERTY(double frameRate READ frameRate NOTIFY frameRateChanged)
    Q_PROPERTY(int videoWidth READ videoWidth NOTIFY videoSizeChanged)
    Q_PROPERTY(int videoHeight READ videoHeight NOTIFY videoSizeChanged)
    Q_PROPERTY(bool loaded READ isLoaded NOTIFY loadedChanged)

public:
    explicit VideoPlayer(QQuickItem *parent = nullptr);
    ~VideoPlayer() override;

    void paint(QPainter *painter) override;

    QString source() const { return m_source; }
    void setSource(const QString &path);

    bool isPlaying() const { return m_playing; }
    void setPlaying(bool playing);

    qint64 position() const { return m_positionMs; }
    void setPosition(qint64 ms);

    qint64 duration() const { return m_durationMs; }
    double frameRate() const { return m_frameRate; }
    int videoWidth() const { return m_videoWidth; }
    int videoHeight() const { return m_videoHeight; }
    bool isLoaded() const { return m_loaded; }

    Q_INVOKABLE void setSegments(const QVariantList &segments);

    Q_INVOKABLE void play();
    Q_INVOKABLE void pause();
    Q_INVOKABLE void togglePlayPause();
    Q_INVOKABLE void seek(qint64 ms);
    Q_INVOKABLE void stepForward();
    Q_INVOKABLE void stepBackward();

signals:
    void sourceChanged();
    void playingChanged();
    void positionChanged();
    void durationChanged();
    void frameRateChanged();
    void videoSizeChanged();
    void loadedChanged();
    void ended();

private:
    void advanceFrame();

    std::unique_ptr<FFmpegDecoder> m_decoder;       // background playback decode
    std::unique_ptr<FFmpegDecoder> m_scrubDecoder;   // main thread scrub/convert

    DecodeWorker m_worker;
    QTimer m_frameTimer;
    QElapsedTimer m_seekThrottle;

    QString m_source;
    QString m_filePath;
    QImage m_currentFrame;
    QRectF m_targetRect;
    bool m_playing = false;
    bool m_loaded = false;
    qint64 m_positionMs = 0;
    qint64 m_durationMs = 0;
    double m_frameRate = 30.0;
    int m_videoWidth = 0;
    int m_videoHeight = 0;

    QList<CutRegion> m_cutRegions;
};

} // namespace screencopy
