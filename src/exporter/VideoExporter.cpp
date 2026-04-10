#include "VideoExporter.h"
#include "FFmpegDecoder.h"
#include "FFmpegEncoder.h"
#include "ProjectManager.h"
#include "TrimModel.h"
#include "EffectRegionModel.h"
#include "filters/CompositeFilter.h"
#include <QDebug>
#include <QFileDialog>
#include <QtMath>

namespace screencopy {

VideoExporter::VideoExporter(QObject *parent)
    : QObject(parent)
{
}

FilterPipeline VideoExporter::createDefaultPipeline()
{
    FilterPipeline pipeline;
    pipeline.addFilter(std::make_unique<CompositeFilter>());
    // Future filters go here:
    // pipeline.addFilter(std::make_unique<ZoomFilter>());
    // pipeline.addFilter(std::make_unique<AnnotationFilter>());
    // pipeline.addFilter(std::make_unique<WatermarkFilter>());
    return pipeline;
}

static int64_t bitrateForQuality(ExportQuality quality, int width, int height, double fps)
{
    int pixels = width * height;
    double boost = fps >= 60 ? 1.7 : 1.0;

    int64_t base;
    if (pixels >= 8294400)       base = 45000000;  // 4K
    else if (pixels >= 3686400)  base = 28000000;  // QHD
    else                         base = 18000000;  // HD

    switch (quality) {
    case ExportQuality::Source: return static_cast<int64_t>(base * boost);
    case ExportQuality::Good:   return static_cast<int64_t>(base * boost * 0.7);
    case ExportQuality::Medium: return static_cast<int64_t>(base * boost * 0.4);
    }
    return base;
}

void VideoExporter::startExport(const QString &inputPath,
                                const QString &outputPath,
                                int qualityIndex)
{
    if (m_exporting)
        return;

    ExportQuality quality;
    switch (qualityIndex) {
    case 0: quality = ExportQuality::Medium; break;
    case 2: quality = ExportQuality::Source; break;
    default: quality = ExportQuality::Good; break;
    }

    // Read current editor state from ProjectManager
    EditorState edState;
    if (m_projectManager)
        edState = m_projectManager->editorState();

    m_exporting = true;
    m_cancelled = false;
    m_progress = 0.0;
    m_statusText = "Preparing...";
    emit exportingChanged();
    emit progressChanged();
    emit statusTextChanged();

    // Copy trim segments and effect regions for worker thread
    QList<TrimSegment> segments;
    if (m_trimModel && m_trimModel->segmentCount() > 0)
        segments = m_trimModel->segments();

    QList<EffectRegion> effects;
    if (m_effectRegions)
        effects = m_effectRegions->regions();

    // Run on worker thread to keep UI responsive
    m_workerThread = QThread::create([this, inputPath, outputPath, edState, quality, segments, effects]() {
        runExport(inputPath, outputPath, edState, quality, segments, effects);
    });

    connect(m_workerThread, &QThread::finished, this, [this, outputPath]() {
        m_workerThread->deleteLater();
        m_workerThread = nullptr;
        bool wasCancelled = m_cancelled.load();
        m_exporting = false;
        emit exportingChanged();

        if (!wasCancelled && m_progress >= 0.99) {
            emit exportFinished(outputPath);
        }
    });

    m_workerThread->start();
}

void VideoExporter::startExportWithDialog(const QString &inputPath, int qualityIndex)
{
    QString outputPath = QFileDialog::getSaveFileName(
        nullptr,
        "Export Video",
        QDir::homePath() + "/output.mp4",
        "MP4 Video (*.mp4)"
    );

    if (outputPath.isEmpty())
        return;

    if (!outputPath.endsWith(".mp4", Qt::CaseInsensitive))
        outputPath += ".mp4";

    startExport(inputPath, outputPath, qualityIndex);
}

void VideoExporter::cancelExport()
{
    m_cancelled = true;
}

// Helper: find active effect regions at a given time
static QList<ActiveEffect> findActiveEffects(const QList<EffectRegion> &regions, qint64 timeMs)
{
    QList<ActiveEffect> active;
    for (const auto &r : regions) {
        if (timeMs >= r.startMs && timeMs <= r.endMs) {
            ActiveEffect ae;
            ae.type = static_cast<int>(r.type);
            ae.startMs = r.startMs;
            ae.endMs = r.endMs;
            ae.settings = r.settings;
            active.append(ae);
        }
    }
    return active;
}

void VideoExporter::runExport(const QString &inputPath,
                              const QString &outputPath,
                              const EditorState &editorState,
                              ExportQuality quality,
                              const QList<TrimSegment> &segments,
                              const QList<EffectRegion> &effectRegions)
{
    // --- 1. Open input ---
    FFmpegDecoder decoder;
    if (!decoder.open(inputPath)) {
        QMetaObject::invokeMethod(this, [this]() {
            m_statusText = "Failed to open video";
            emit statusTextChanged();
            emit exportError("Failed to open input video");
        }, Qt::QueuedConnection);
        return;
    }

    int srcWidth = decoder.width();
    int srcHeight = decoder.height();
    double fps = decoder.frameRate();
    if (fps <= 0) fps = 30.0;

    // Calculate total trimmed duration from segments
    qint64 trimmedDurationMs = 0;
    QList<TrimSegment> segs = segments;
    if (segs.isEmpty()) {
        // No trim — export full video
        segs.append({0, decoder.durationMs()});
    }
    for (const auto &seg : segs)
        trimmedDurationMs += seg.durationMs();

    int totalFrames = qRound(trimmedDurationMs / 1000.0 * fps);
    if (totalFrames <= 0) totalFrames = 1;

    qDebug() << "Export:" << srcWidth << "x" << srcHeight << "@" << fps
             << "fps," << segs.size() << "segments,"
             << trimmedDurationMs << "ms trimmed," << totalFrames << "frames";

    // --- 2. Open encoder ---
    int outWidth = srcWidth;
    int outHeight = srcHeight;
    int64_t bitrate = bitrateForQuality(quality, outWidth, outHeight, fps);

    FFmpegEncoder encoder;
    if (!encoder.open(outputPath, outWidth, outHeight, fps, bitrate)) {
        QMetaObject::invokeMethod(this, [this]() {
            m_statusText = "Failed to create output";
            emit statusTextChanged();
            emit exportError("Failed to open encoder");
        }, Qt::QueuedConnection);
        return;
    }

    // --- 3. Build filter pipeline ---
    FilterPipeline pipeline = createDefaultPipeline();
    qDebug() << "Export pipeline:" << pipeline.filterNames();

    QMetaObject::invokeMethod(this, [this]() {
        m_statusText = "Encoding...";
        emit statusTextChanged();
    }, Qt::QueuedConnection);

    // --- 4. Process frames — iterate over trim segments ---
    int frameIndex = 0;

    for (int segIdx = 0; segIdx < segs.size() && !m_cancelled.load(); ++segIdx) {
        const auto &seg = segs[segIdx];

        // Seek to segment start
        decoder.seekTo(seg.startMs);

        // Decode frames within this segment
        while (!m_cancelled.load()) {
            QImage rawFrame = decoder.decodeNextFrame();
            if (rawFrame.isNull())
                break;

            qint64 framePts = decoder.lastFramePtsMs();

            // Stop if we've gone past this segment's end
            if (framePts > seg.endMs)
                break;

            // Skip frames before segment start (after seek lands on earlier keyframe)
            if (framePts < seg.startMs)
                continue;

            // Build filter context with active effects at this timestamp
            FilterContext ctx;
            ctx.timeMs = framePts;
            ctx.outputWidth = outWidth;
            ctx.outputHeight = outHeight;
            ctx.videoFrame = rawFrame;
            ctx.editor = &editorState;
            ctx.activeEffects = findActiveEffects(effectRegions, framePts);

            // Run through pipeline
            QImage composited = pipeline.process(rawFrame, ctx);

            // Encode
            if (!encoder.writeFrame(composited)) {
                QMetaObject::invokeMethod(this, [this]() {
                    emit exportError("Failed to encode frame");
                }, Qt::QueuedConnection);
                goto done;
            }

            frameIndex++;

            // Update progress
            if (frameIndex % 5 == 0 || frameIndex >= totalFrames) {
                double prog = qMin(1.0, static_cast<double>(frameIndex) / totalFrames);
                QMetaObject::invokeMethod(this, [this, prog, frameIndex, totalFrames]() {
                    m_progress = prog;
                    m_statusText = QString("Encoding frame %1/%2...").arg(frameIndex).arg(totalFrames);
                    emit progressChanged();
                    emit statusTextChanged();
                }, Qt::QueuedConnection);
            }
        }
    }

done:
    // --- 5. Finalize ---
    if (!m_cancelled.load()) {
        encoder.finish();
        QMetaObject::invokeMethod(this, [this]() {
            m_progress = 1.0;
            m_statusText = "Done";
            emit progressChanged();
            emit statusTextChanged();
        }, Qt::QueuedConnection);
    }

    encoder.close();
}

} // namespace screencopy
