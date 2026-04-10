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
    bool isHardwareAccelerated() const { return m_hwDeviceCtx != nullptr; }
    qint64 durationMs() const;
    int width() const;
    int height() const;
    double frameRate() const;
    qint64 lastFramePtsMs() const { return m_lastPtsMs; }

    // Seek to keyframe at or before ms (flushes decoder — expensive with HW accel)
    bool seekTo(qint64 ms);

    // Decode and convert the next frame to QImage
    QImage decodeNextFrame();

    // Seek + decode to exact frame at ms (uses cache, only converts target frame)
    QImage decodeFrameAt(qint64 ms);

    // Decode forward from current position without seeking (no flush — fast for short skips)
    QImage skipTo(qint64 targetMs);

    // Decode next raw AVFrame without QImage conversion (caller must av_frame_free)
    AVFrame *decodeNextRawFrame();

    // Convert a raw AVFrame to QImage
    QImage frameToImage(AVFrame *frame);

private:
    bool openHardwareDecoder(const AVCodecParameters *codecpar);
    bool openSoftwareDecoder(const AVCodecParameters *codecpar);
    QImage seekAndDecodeTarget(qint64 targetMs);
    QImage convertFrame(AVFrame *frame);
    QImage transferHwFrame(AVFrame *hwFrame);
    qint64 framePtsMs() const;

    AVFormatContext *m_formatCtx = nullptr;
    AVCodecContext *m_codecCtx = nullptr;
    SwsContext *m_swsCtx = nullptr;
    enum AVPixelFormat m_swsInputFmt = AV_PIX_FMT_NONE;
    int m_swsInputW = 0;
    int m_swsInputH = 0;
    AVBufferRef *m_hwDeviceCtx = nullptr;
    AVFrame *m_frame = nullptr;
    AVFrame *m_swFrame = nullptr;
    AVPacket *m_packet = nullptr;
    int m_videoStreamIdx = -1;
    qint64 m_lastPtsMs = 0;
    enum AVPixelFormat m_hwPixFmt = AV_PIX_FMT_NONE;

    QCache<qint64, QImage> m_frameCache;
};

} // namespace screencopy
