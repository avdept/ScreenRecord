#include "FFmpegDecoder.h"
#include <QDebug>

namespace screencopy {

FFmpegDecoder::FFmpegDecoder(QObject *parent)
    : QObject(parent)
    , m_frameCache(100)
{
}

FFmpegDecoder::~FFmpegDecoder()
{
    close();
}

bool FFmpegDecoder::open(const QString &filePath)
{
    close();

    if (avformat_open_input(&m_formatCtx, filePath.toUtf8().constData(), nullptr, nullptr) < 0) {
        qWarning() << "FFmpegDecoder: failed to open" << filePath;
        return false;
    }

    if (avformat_find_stream_info(m_formatCtx, nullptr) < 0) {
        close();
        return false;
    }

    for (unsigned i = 0; i < m_formatCtx->nb_streams; i++) {
        if (m_formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            m_videoStreamIdx = static_cast<int>(i);
            break;
        }
    }
    if (m_videoStreamIdx < 0) { close(); return false; }

    auto *codecpar = m_formatCtx->streams[m_videoStreamIdx]->codecpar;
    const AVCodec *codec = nullptr;

#ifdef Q_OS_MACOS
    // Prefer hevc_videotoolbox on macOS — the default software HEVC decoder
    // ignores the alpha auxiliary layer in HEVC-with-alpha files, producing
    // opaque output. The VideoToolbox decoder handles alpha natively.
    if (codecpar->codec_id == AV_CODEC_ID_HEVC) {
        codec = avcodec_find_decoder_by_name("hevc_videotoolbox");
    }
#endif
    if (!codec)
        codec = avcodec_find_decoder(codecpar->codec_id);
    if (!codec) { close(); return false; }

    m_codecCtx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(m_codecCtx, codecpar);
    m_codecCtx->thread_count = 0;  // auto
    m_codecCtx->thread_type = FF_THREAD_FRAME | FF_THREAD_SLICE;

#ifdef Q_OS_MACOS
    // macOS only: prefer alpha-aware pixel formats when the HEVC decoder
    // offers a choice. Without this, FFmpeg's software hevc decoder outputs
    // yuv420p for HEVC-with-alpha files recorded via ScreenCaptureKit — the
    // alpha auxiliary layer is silently dropped.
    //
    // Gated to macOS because on Linux the decoder may be negotiating a
    // hardware-accelerated surface format (e.g. VAAPI). Preferring a software
    // alpha format there would force SW decoding and defeat hwaccel. Linux
    // recordings come from gpu-screen-recorder as plain H.264/HEVC without
    // alpha, so the callback serves no purpose on that platform.
    m_codecCtx->get_format = [](AVCodecContext *, const AVPixelFormat *fmts) -> AVPixelFormat {
        for (int i = 0; fmts[i] != AV_PIX_FMT_NONE; ++i) {
            if (fmts[i] == AV_PIX_FMT_YUVA420P
                || fmts[i] == AV_PIX_FMT_YUVA422P
                || fmts[i] == AV_PIX_FMT_YUVA444P
                || fmts[i] == AV_PIX_FMT_BGRA
                || fmts[i] == AV_PIX_FMT_RGBA) {
                return fmts[i];
            }
        }
        return fmts[0];
    };
#endif

    if (avcodec_open2(m_codecCtx, codec, nullptr) < 0) {
        avcodec_free_context(&m_codecCtx);
        close();
        return false;
    }

    m_frame = av_frame_alloc();
    m_packet = av_packet_alloc();

    return true;
}

void FFmpegDecoder::close()
{
    m_frameCache.clear();
    if (m_packet) { av_packet_free(&m_packet); m_packet = nullptr; }
    if (m_frame) { av_frame_free(&m_frame); m_frame = nullptr; }
    if (m_swsCtx) { sws_freeContext(m_swsCtx); m_swsCtx = nullptr; }
    if (m_codecCtx) { avcodec_free_context(&m_codecCtx); m_codecCtx = nullptr; }
    if (m_formatCtx) { avformat_close_input(&m_formatCtx); m_formatCtx = nullptr; }
    m_videoStreamIdx = -1;
    m_lastPtsMs = 0;
    m_swsInputFmt = AV_PIX_FMT_NONE;
    m_swsInputW = 0;
    m_swsInputH = 0;
}

qint64 FFmpegDecoder::durationMs() const
{
    if (!m_formatCtx) return 0;
    return m_formatCtx->duration / 1000;
}

int FFmpegDecoder::width() const { return m_codecCtx ? m_codecCtx->width : 0; }
int FFmpegDecoder::height() const { return m_codecCtx ? m_codecCtx->height : 0; }

double FFmpegDecoder::frameRate() const
{
    if (!m_formatCtx || m_videoStreamIdx < 0) return 0;
    auto r = m_formatCtx->streams[m_videoStreamIdx]->avg_frame_rate;
    return r.den > 0 ? static_cast<double>(r.num) / r.den : 0;
}

qint64 FFmpegDecoder::framePtsMs() const
{
    if (!m_frame || !m_formatCtx || m_videoStreamIdx < 0) return 0;
    auto *stream = m_formatCtx->streams[m_videoStreamIdx];
    auto tb = stream->time_base;
    if (m_frame->pts == AV_NOPTS_VALUE) return 0;
    return m_frame->pts * 1000 * tb.num / tb.den;
}

bool FFmpegDecoder::seekTo(qint64 ms)
{
    if (!m_formatCtx || m_videoStreamIdx < 0) return false;

    auto *stream = m_formatCtx->streams[m_videoStreamIdx];
    auto tb = stream->time_base;
    int64_t targetTs = ms * tb.den / (1000 * tb.num);

    int ret = av_seek_frame(m_formatCtx, m_videoStreamIdx, targetTs, AVSEEK_FLAG_BACKWARD);
    if (ret < 0) return false;

    avcodec_flush_buffers(m_codecCtx);
    return true;
}

QImage FFmpegDecoder::decodeFrameAt(qint64 ms)
{
    for (qint64 d = 0; d <= 50; d += 1) {
        if (auto *cached = m_frameCache.object(ms + d)) return *cached;
        if (d > 0) {
            if (auto *cached = m_frameCache.object(ms - d)) return *cached;
        }
    }

    QImage result = seekAndDecodeTarget(ms);

    if (!result.isNull())
        m_frameCache.insert(m_lastPtsMs, new QImage(result));

    return result;
}

QImage FFmpegDecoder::skipTo(qint64 targetMs)
{
    if (!m_formatCtx || !m_codecCtx) return {};

    auto *stream = m_formatCtx->streams[m_videoStreamIdx];
    auto tb = stream->time_base;
    int64_t targetPts = targetMs * tb.den / (1000 * tb.num);

    QImage result;
    int maxPackets = 500;

    while (maxPackets-- > 0) {
        int ret = av_read_frame(m_formatCtx, m_packet);
        if (ret < 0) break;

        if (m_packet->stream_index != m_videoStreamIdx) {
            av_packet_unref(m_packet);
            continue;
        }

        avcodec_send_packet(m_codecCtx, m_packet);
        av_packet_unref(m_packet);

        while (avcodec_receive_frame(m_codecCtx, m_frame) == 0) {
            m_lastPtsMs = framePtsMs();

            if (m_frame->pts >= targetPts) {
                QImage img = convertFrame(m_frame);
                if (!img.isNull())
                    m_frameCache.insert(m_lastPtsMs, new QImage(img));
                return img;
            }
        }
    }

    return result;
}

QImage FFmpegDecoder::seekAndDecodeTarget(qint64 targetMs)
{
    if (!m_formatCtx || !m_codecCtx) return {};
    if (!seekTo(targetMs)) return {};

    auto *stream = m_formatCtx->streams[m_videoStreamIdx];
    auto tb = stream->time_base;
    int64_t targetPts = targetMs * tb.den / (1000 * tb.num);

    for (int i = 0; i < 500; i++) {
        int ret = av_read_frame(m_formatCtx, m_packet);
        if (ret < 0) break;

        if (m_packet->stream_index != m_videoStreamIdx) {
            av_packet_unref(m_packet);
            continue;
        }

        avcodec_send_packet(m_codecCtx, m_packet);
        av_packet_unref(m_packet);

        while (avcodec_receive_frame(m_codecCtx, m_frame) == 0) {
            if (m_frame->pts >= targetPts) {
                m_lastPtsMs = framePtsMs();
                return convertFrame(m_frame);
            }
        }
    }
    return {};
}

QImage FFmpegDecoder::convertFrame(AVFrame *frame)
{
    if (!frame || frame->width <= 0) return {};

    auto srcFmt = static_cast<AVPixelFormat>(frame->format);

    if (!m_swsCtx || srcFmt != m_swsInputFmt
        || frame->width != m_swsInputW || frame->height != m_swsInputH) {
        if (m_swsCtx) sws_freeContext(m_swsCtx);
        m_swsCtx = sws_getContext(
            frame->width, frame->height, srcFmt,
            frame->width, frame->height, AV_PIX_FMT_RGBA,
            SWS_BILINEAR, nullptr, nullptr, nullptr);
        m_swsInputFmt = srcFmt;
        m_swsInputW = frame->width;
        m_swsInputH = frame->height;
    }

    if (!m_swsCtx) return {};

    QImage image(frame->width, frame->height, QImage::Format_RGBA8888);
    uint8_t *dstData[1] = { image.bits() };
    int dstLinesize[1] = { static_cast<int>(image.bytesPerLine()) };
    sws_scale(m_swsCtx, frame->data, frame->linesize,
              0, frame->height, dstData, dstLinesize);
    return image;
}

QImage FFmpegDecoder::decodeNextFrame()
{
    if (!m_formatCtx || !m_codecCtx) return {};

    // First try to get a buffered frame
    if (avcodec_receive_frame(m_codecCtx, m_frame) == 0) {
        m_lastPtsMs = framePtsMs();
        QImage img = convertFrame(m_frame);
        if (!img.isNull())
            m_frameCache.insert(m_lastPtsMs, new QImage(img));
        return img;
    }

    while (av_read_frame(m_formatCtx, m_packet) >= 0) {
        if (m_packet->stream_index != m_videoStreamIdx) {
            av_packet_unref(m_packet);
            continue;
        }

        avcodec_send_packet(m_codecCtx, m_packet);
        av_packet_unref(m_packet);

        if (avcodec_receive_frame(m_codecCtx, m_frame) == 0) {
            m_lastPtsMs = framePtsMs();
            QImage img = convertFrame(m_frame);
            if (!img.isNull())
                m_frameCache.insert(m_lastPtsMs, new QImage(img));
            return img;
        }
    }
    return {};
}

AVFrame *FFmpegDecoder::decodeNextRawFrame()
{
    if (!m_formatCtx || !m_codecCtx) return nullptr;

    // First try to get a buffered frame
    if (avcodec_receive_frame(m_codecCtx, m_frame) == 0) {
        m_lastPtsMs = framePtsMs();
        return av_frame_clone(m_frame);
    }

    while (av_read_frame(m_formatCtx, m_packet) >= 0) {
        if (m_packet->stream_index != m_videoStreamIdx) {
            av_packet_unref(m_packet);
            continue;
        }

        avcodec_send_packet(m_codecCtx, m_packet);
        av_packet_unref(m_packet);

        if (avcodec_receive_frame(m_codecCtx, m_frame) == 0) {
            m_lastPtsMs = framePtsMs();
            return av_frame_clone(m_frame);
        }
    }
    return nullptr;
}

QImage FFmpegDecoder::frameToImage(AVFrame *frame)
{
    if (!frame) return {};
    return convertFrame(frame);
}

} // namespace screencopy
