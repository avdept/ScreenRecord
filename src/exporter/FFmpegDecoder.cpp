#include "FFmpegDecoder.h"
#include <QDebug>

extern "C" {
#include <libavutil/hwcontext.h>
}

namespace screencopy {

static const AVHWDeviceType s_hwTypes[] = {
    AV_HWDEVICE_TYPE_VAAPI,
    AV_HWDEVICE_TYPE_CUDA,
    AV_HWDEVICE_TYPE_VIDEOTOOLBOX,
    AV_HWDEVICE_TYPE_D3D11VA,
    AV_HWDEVICE_TYPE_VDPAU,
    AV_HWDEVICE_TYPE_NONE
};

static enum AVPixelFormat getHwFormat(AVCodecContext *ctx, const enum AVPixelFormat *pix_fmts)
{
    auto hwPixFmt = static_cast<AVPixelFormat>(reinterpret_cast<intptr_t>(ctx->opaque));
    for (auto *p = pix_fmts; *p != AV_PIX_FMT_NONE; p++) {
        if (*p == hwPixFmt)
            return *p;
    }
    return pix_fmts[0];
}

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

    if (!openHardwareDecoder(codecpar)) {
        if (!openSoftwareDecoder(codecpar)) {
            close();
            return false;
        }
    }

    m_frame = av_frame_alloc();
    m_swFrame = av_frame_alloc();
    m_packet = av_packet_alloc();


    return true;
}

bool FFmpegDecoder::openHardwareDecoder(const AVCodecParameters *codecpar)
{
    auto *codec = avcodec_find_decoder(codecpar->codec_id);
    if (!codec) return false;

    for (int i = 0; s_hwTypes[i] != AV_HWDEVICE_TYPE_NONE; i++) {
        auto hwType = s_hwTypes[i];

        m_hwPixFmt = AV_PIX_FMT_NONE;
        for (int j = 0;; j++) {
            auto *config = avcodec_get_hw_config(codec, j);
            if (!config) break;
            if (config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX
                && config->device_type == hwType) {
                m_hwPixFmt = config->pix_fmt;
                break;
            }
        }
        if (m_hwPixFmt == AV_PIX_FMT_NONE) continue;

        AVBufferRef *hwCtx = nullptr;
        if (av_hwdevice_ctx_create(&hwCtx, hwType, nullptr, nullptr, 0) < 0)
            continue;

        m_codecCtx = avcodec_alloc_context3(codec);
        avcodec_parameters_to_context(m_codecCtx, codecpar);
        m_codecCtx->hw_device_ctx = av_buffer_ref(hwCtx);
        m_codecCtx->opaque = reinterpret_cast<void *>(static_cast<intptr_t>(m_hwPixFmt));
        m_codecCtx->get_format = getHwFormat;

        if (avcodec_open2(m_codecCtx, codec, nullptr) == 0) {
            m_hwDeviceCtx = hwCtx;
            return true;
        }

        avcodec_free_context(&m_codecCtx);
        av_buffer_unref(&hwCtx);
    }

    return false;
}

bool FFmpegDecoder::openSoftwareDecoder(const AVCodecParameters *codecpar)
{
    auto *codec = avcodec_find_decoder(codecpar->codec_id);
    if (!codec) return false;

    m_codecCtx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(m_codecCtx, codecpar);
    m_codecCtx->thread_count = 0;
    m_codecCtx->thread_type = FF_THREAD_FRAME | FF_THREAD_SLICE;

    if (avcodec_open2(m_codecCtx, codec, nullptr) < 0) {
        avcodec_free_context(&m_codecCtx);
        return false;
    }

    m_hwPixFmt = AV_PIX_FMT_NONE;
    return true;
}

void FFmpegDecoder::close()
{
    m_frameCache.clear();
    if (m_packet) { av_packet_free(&m_packet); m_packet = nullptr; }
    if (m_swFrame) { av_frame_free(&m_swFrame); m_swFrame = nullptr; }
    if (m_frame) { av_frame_free(&m_frame); m_frame = nullptr; }
    if (m_swsCtx) { sws_freeContext(m_swsCtx); m_swsCtx = nullptr; }
    if (m_codecCtx) { avcodec_free_context(&m_codecCtx); m_codecCtx = nullptr; }
    if (m_hwDeviceCtx) { av_buffer_unref(&m_hwDeviceCtx); m_hwDeviceCtx = nullptr; }
    if (m_formatCtx) { avformat_close_input(&m_formatCtx); m_formatCtx = nullptr; }
    m_videoStreamIdx = -1;
    m_lastPtsMs = 0;
    m_hwPixFmt = AV_PIX_FMT_NONE;
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
    // Check cache — look within 50ms radius (covers ~3 frames at 60fps)
    for (qint64 d = 0; d <= 50; d += 1) {
        if (auto *cached = m_frameCache.object(ms + d)) return *cached;
        if (d > 0) {
            if (auto *cached = m_frameCache.object(ms - d)) return *cached;
        }
    }

    // Cache miss — decode the target frame
    QImage result = seekAndDecodeTarget(ms);

    // Cache the result at the actual PTS for future lookups
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

    // NO seek, NO flush — just decode forward from current position,
    // discarding frames until we reach the target PTS.
    // This preserves decoder state so it's much faster for short skips.
    QImage result;
    int maxPackets = 500; // safety limit

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
                // Reached target — convert and return this frame
                QImage img;
                if (m_hwDeviceCtx && m_frame->format == m_hwPixFmt)
                    img = transferHwFrame(m_frame);
                else
                    img = convertFrame(m_frame);

                if (!img.isNull())
                    m_frameCache.insert(m_lastPtsMs, new QImage(img));
                return img;
            }
            // Not there yet — discard (but HW decode is fast, ~0.1ms per frame)
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

    // Decode forward but DON'T convert to QImage until we reach target.
    // This skips the expensive sws_scale for all intermediate frames.
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
                // This is the target frame — convert only this one
                m_lastPtsMs = framePtsMs();
                QImage img;
                if (m_hwDeviceCtx && m_frame->format == m_hwPixFmt)
                    img = transferHwFrame(m_frame);
                else
                    img = convertFrame(m_frame);
                return img;
            }
            // Not there yet — just discard, no conversion
        }
    }
    return {};
}

QImage FFmpegDecoder::transferHwFrame(AVFrame *hwFrame)
{
    if (av_hwframe_transfer_data(m_swFrame, hwFrame, 0) < 0)
        return {};
    m_swFrame->pts = hwFrame->pts;
    return convertFrame(m_swFrame);
}

QImage FFmpegDecoder::convertFrame(AVFrame *frame)
{
    if (!frame || frame->width <= 0) return {};

    auto srcFmt = static_cast<AVPixelFormat>(frame->format);

    // Recreate sws context when pixel format or size changes
    // (HW transfer produces NV12, software decode produces YUV420P, etc.)
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

    while (av_read_frame(m_formatCtx, m_packet) >= 0) {
        if (m_packet->stream_index != m_videoStreamIdx) {
            av_packet_unref(m_packet);
            continue;
        }

        if (avcodec_send_packet(m_codecCtx, m_packet) < 0) {
            av_packet_unref(m_packet);
            continue;
        }
        av_packet_unref(m_packet);

        if (avcodec_receive_frame(m_codecCtx, m_frame) == 0) {
            m_lastPtsMs = framePtsMs();

            QImage img;
            if (m_hwDeviceCtx && m_frame->format == m_hwPixFmt)
                img = transferHwFrame(m_frame);
            else
                img = convertFrame(m_frame);

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

    while (av_read_frame(m_formatCtx, m_packet) >= 0) {
        if (m_packet->stream_index != m_videoStreamIdx) {
            av_packet_unref(m_packet);
            continue;
        }

        if (avcodec_send_packet(m_codecCtx, m_packet) < 0) {
            av_packet_unref(m_packet);
            continue;
        }
        av_packet_unref(m_packet);

        if (avcodec_receive_frame(m_codecCtx, m_frame) == 0) {
            m_lastPtsMs = framePtsMs();

            // For HW frames, transfer to SW immediately (needed for cross-thread clone)
            if (m_hwDeviceCtx && m_frame->format == m_hwPixFmt) {
                if (av_hwframe_transfer_data(m_swFrame, m_frame, 0) < 0)
                    return nullptr;
                m_swFrame->pts = m_frame->pts;
                return av_frame_clone(m_swFrame);
            }

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
