#pragma once

#include <QObject>
#include <QImage>
#include <QString>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

namespace screencopy {

class FFmpegEncoder : public QObject
{
    Q_OBJECT

public:
    explicit FFmpegEncoder(QObject *parent = nullptr);
    ~FFmpegEncoder() override;

    bool open(const QString &outputPath, int width, int height, double fps, int64_t bitrate);
    bool writeFrame(const QImage &frame);
    bool finish();
    void close();

private:
    AVFormatContext *m_formatCtx = nullptr;
    AVCodecContext *m_codecCtx = nullptr;
    SwsContext *m_swsCtx = nullptr;
    AVStream *m_stream = nullptr;
    AVFrame *m_frame = nullptr;
    AVPacket *m_packet = nullptr;
    int64_t m_frameCount = 0;
};

} // namespace screencopy
