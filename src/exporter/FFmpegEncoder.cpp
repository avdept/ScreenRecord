#include "FFmpegEncoder.h"
#include <QDebug>

namespace screencopy {

FFmpegEncoder::FFmpegEncoder(QObject *parent)
    : QObject(parent)
{
}

FFmpegEncoder::~FFmpegEncoder()
{
    close();
}

bool FFmpegEncoder::open(const QString &outputPath, int width, int height, double fps, int64_t bitrate)
{
    close();

    avformat_alloc_output_context2(&m_formatCtx, nullptr, nullptr, outputPath.toUtf8().constData());
    if (!m_formatCtx) return false;

    auto *codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!codec) return false;

    m_stream = avformat_new_stream(m_formatCtx, codec);
    m_codecCtx = avcodec_alloc_context3(codec);

    m_codecCtx->width = width;
    m_codecCtx->height = height;
    m_codecCtx->time_base = {1, static_cast<int>(fps)};
    m_codecCtx->framerate = {static_cast<int>(fps), 1};
    m_codecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
    m_codecCtx->bit_rate = bitrate;
    m_codecCtx->gop_size = 12;

    if (m_formatCtx->oformat->flags & AVFMT_GLOBALHEADER)
        m_codecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    if (avcodec_open2(m_codecCtx, codec, nullptr) < 0) return false;
    avcodec_parameters_from_context(m_stream->codecpar, m_codecCtx);
    m_stream->time_base = m_codecCtx->time_base;

    m_frame = av_frame_alloc();
    m_frame->format = m_codecCtx->pix_fmt;
    m_frame->width = width;
    m_frame->height = height;
    av_frame_get_buffer(m_frame, 0);

    m_packet = av_packet_alloc();

    if (!(m_formatCtx->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&m_formatCtx->pb, outputPath.toUtf8().constData(), AVIO_FLAG_WRITE) < 0)
            return false;
    }

    avformat_write_header(m_formatCtx, nullptr);
    m_frameCount = 0;
    return true;
}

bool FFmpegEncoder::writeFrame(const QImage &frame)
{
    if (!m_codecCtx || frame.isNull()) return false;

    auto rgbaImage = frame.convertToFormat(QImage::Format_RGBA8888);

    if (!m_swsCtx) {
        m_swsCtx = sws_getContext(
            rgbaImage.width(), rgbaImage.height(), AV_PIX_FMT_RGBA,
            m_codecCtx->width, m_codecCtx->height, AV_PIX_FMT_YUV420P,
            SWS_BILINEAR, nullptr, nullptr, nullptr);
    }

    const uint8_t *srcData[1] = { rgbaImage.constBits() };
    int srcLinesize[1] = { static_cast<int>(rgbaImage.bytesPerLine()) };
    sws_scale(m_swsCtx, srcData, srcLinesize, 0, rgbaImage.height(),
              m_frame->data, m_frame->linesize);

    m_frame->pts = m_frameCount++;

    avcodec_send_frame(m_codecCtx, m_frame);

    while (avcodec_receive_packet(m_codecCtx, m_packet) == 0) {
        av_packet_rescale_ts(m_packet, m_codecCtx->time_base, m_stream->time_base);
        m_packet->stream_index = m_stream->index;
        av_interleaved_write_frame(m_formatCtx, m_packet);
        av_packet_unref(m_packet);
    }

    return true;
}

bool FFmpegEncoder::finish()
{
    if (!m_codecCtx) return false;

    // Flush encoder
    avcodec_send_frame(m_codecCtx, nullptr);
    while (avcodec_receive_packet(m_codecCtx, m_packet) == 0) {
        av_packet_rescale_ts(m_packet, m_codecCtx->time_base, m_stream->time_base);
        m_packet->stream_index = m_stream->index;
        av_interleaved_write_frame(m_formatCtx, m_packet);
        av_packet_unref(m_packet);
    }

    av_write_trailer(m_formatCtx);
    return true;
}

void FFmpegEncoder::close()
{
    if (m_packet) { av_packet_free(&m_packet); m_packet = nullptr; }
    if (m_frame) { av_frame_free(&m_frame); m_frame = nullptr; }
    if (m_swsCtx) { sws_freeContext(m_swsCtx); m_swsCtx = nullptr; }
    if (m_codecCtx) { avcodec_free_context(&m_codecCtx); m_codecCtx = nullptr; }
    if (m_formatCtx) {
        if (!(m_formatCtx->oformat->flags & AVFMT_NOFILE))
            avio_closep(&m_formatCtx->pb);
        avformat_free_context(m_formatCtx);
        m_formatCtx = nullptr;
    }
    m_stream = nullptr;
    m_frameCount = 0;
}

} // namespace screencopy
