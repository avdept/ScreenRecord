#pragma once

#include <QObject>
#include <QImage>
#include <QString>
#include <QCache>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

namespace screencopy {

class FFmpegDecoder : public QObject
{
    Q_OBJECT

public:
    explicit FFmpegDecoder(QObject *parent = nullptr);
    ~FFmpegDecoder() override;

    bool open(const QString &filePath);
    void close();

    bool isOpen() const { return m_formatCtx != nullptr; }
    qint64 durationMs() const;
    int width() const;
    int height() const;
    double frameRate() const;
    qint64 lastFramePtsMs() const { return m_lastPtsMs; }

    bool seekTo(qint64 ms);
    QImage decodeNextFrame();
    QImage decodeFrameAt(qint64 ms);
    QImage skipTo(qint64 targetMs);

    AVFrame *decodeNextRawFrame();
    QImage frameToImage(AVFrame *frame);

private:
    QImage seekAndDecodeTarget(qint64 targetMs);
    QImage convertFrame(AVFrame *frame);
    qint64 framePtsMs() const;

    AVFormatContext *m_formatCtx = nullptr;
    AVCodecContext *m_codecCtx = nullptr;
    SwsContext *m_swsCtx = nullptr;
    enum AVPixelFormat m_swsInputFmt = AV_PIX_FMT_NONE;
    int m_swsInputW = 0;
    int m_swsInputH = 0;
    AVFrame *m_frame = nullptr;
    AVPacket *m_packet = nullptr;
    int m_videoStreamIdx = -1;
    qint64 m_lastPtsMs = 0;

    QCache<qint64, QImage> m_frameCache;
};

} // namespace screencopy
