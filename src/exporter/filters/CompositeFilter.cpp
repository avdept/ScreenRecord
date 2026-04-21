#include "CompositeFilter.h"
#include "ProjectData.h"
#include <QPainter>
#include <QPainterPath>
#include <QtMath>
#include <algorithm>

namespace screencopy {

CompositeFilter::CompositeFilter()
    : m_wallpaperCache(4)
{
}

QImage CompositeFilter::process(const QImage &input, const FilterContext &ctx)
{
    Q_UNUSED(input);

    const auto &ed = *ctx.editor;
    int w = ctx.outputWidth;
    int h = ctx.outputHeight;

    // --- 1. Background ---
    QImage canvas(w, h, QImage::Format_ARGB32_Premultiplied);

    if (ed.wallpaper.startsWith('#')) {
        canvas.fill(QColor(ed.wallpaper));
    } else {
        QImage wp = loadWallpaper(ed.wallpaper, QSize(w, h));
        if (wp.isNull()) {
            canvas.fill(QColor("#1a1a2e"));
        } else {
            QPainter bgp(&canvas);
            bgp.setRenderHint(QPainter::SmoothPixmapTransform);
            // Crop-fit
            QSizeF scaled = QSizeF(wp.size()).scaled(QSizeF(w, h), Qt::KeepAspectRatioByExpanding);
            double sx = (wp.width() - scaled.width()) / 2.0;
            double sy = (wp.height() - scaled.height()) / 2.0;
            bgp.drawImage(QRectF(0, 0, w, h), wp, QRectF(sx, sy, scaled.width(), scaled.height()));
            bgp.end();
        }
    }

    // --- 2. Compute video rect with padding ---
    double paddingScale = 1.0 - (ed.padding / 100.0) * 0.4;
    double vidW = w * paddingScale;
    double vidH = h * paddingScale;
    double vidX = (w - vidW) / 2.0;
    double vidY = (h - vidH) / 2.0;
    QRectF videoRect(vidX, vidY, vidW, vidH);
    double radius = ed.borderRadius;

    // --- 3. Create rounded video frame ---
    QImage videoRounded(qRound(vidW), qRound(vidH), QImage::Format_ARGB32_Premultiplied);
    videoRounded.fill(Qt::transparent);
    {
        QPainter vp(&videoRounded);
        vp.setRenderHint(QPainter::Antialiasing);
        vp.setRenderHint(QPainter::SmoothPixmapTransform);

        if (radius > 0.1) {
            QPainterPath path;
            path.addRoundedRect(QRectF(0, 0, vidW, vidH), radius, radius);
            vp.setClipPath(path);
        }

        vp.drawImage(QRectF(0, 0, vidW, vidH), ctx.videoFrame);
        vp.end();
    }

    // --- 4. Shadow ---
    if (ed.shadowIntensity > 0.01) {
        double si = ed.shadowIntensity;
        // Scale shadow to match QML MultiEffect shadowScale: 1.0 + si * 0.05
        double shadowScale = 1.0 + si * 0.05;
        QSize scaledSize(qRound(vidW * shadowScale), qRound(vidH * shadowScale));

        double refSize = std::min(vidW, vidH);
        QImage shadow = createShadow(scaledSize, radius * shadowScale, si, refSize);
        double shadowOffsetY = si * refSize * 0.025;

        QPainter sp(&canvas);
        sp.setRenderHint(QPainter::SmoothPixmapTransform);
        // Shadow is larger than video — center it, offset downward
        double dx = vidX - (shadow.width() - vidW) / 2.0;
        double dy = vidY - (shadow.height() - vidH) / 2.0 + shadowOffsetY;
        sp.drawImage(QPointF(dx, dy), shadow);
        sp.end();
    }

    // --- 5. Draw video onto canvas ---
    {
        QPainter p(&canvas);
        p.setRenderHint(QPainter::SmoothPixmapTransform);
        p.drawImage(videoRect.topLeft(), videoRounded);
        p.end();
    }

    return canvas;
}

QImage CompositeFilter::loadWallpaper(const QString &wpName, const QSize &size)
{
    QString key = wpName + QString("_%1x%2").arg(size.width()).arg(size.height());

    if (auto *cached = m_wallpaperCache.object(key))
        return *cached;

    QString path = QString(":/ScreenCopy/resources/wallpapers/%1.jpg").arg(wpName);
    QImage img(path);
    if (img.isNull())
        return {};

    auto *scaled = new QImage(img.scaled(size, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
    m_wallpaperCache.insert(key, scaled);
    return *scaled;
}

QImage CompositeFilter::createShadow(const QSize &videoSize, double radius, double intensity, double refSize)
{
    // Scale blur radius with video dimensions to match QML MultiEffect appearance
    int blurRadius = qRound(intensity * refSize * 0.035);
    // 3-pass box blur spreads up to 3*radius, so we need 3*blurRadius padding per side
    int expand = blurRadius * 6;
    QSize shadowSize(videoSize.width() + expand, videoSize.height() + expand);

    QImage shadow(shadowSize, QImage::Format_ARGB32_Premultiplied);
    shadow.fill(Qt::transparent);

    // Draw a filled rounded rect as the shadow shape
    QPainter sp(&shadow);
    sp.setRenderHint(QPainter::Antialiasing);
    sp.setBrush(QColor(0, 0, 0, qRound(intensity * 180)));
    sp.setPen(Qt::NoPen);

    QRectF rect(expand / 2.0, expand / 2.0, videoSize.width(), videoSize.height());
    if (radius > 0.1) {
        sp.drawRoundedRect(rect, radius, radius);
    } else {
        sp.drawRect(rect);
    }
    sp.end();

    // Apply box blur (fast approximation of Gaussian)
    if (blurRadius > 0)
        blurImage(shadow, blurRadius);

    return shadow;
}

void CompositeFilter::blurImage(QImage &image, int radius)
{
    // Fast box blur — 3 passes approximates Gaussian
    if (radius < 1) return;

    int w = image.width();
    int h = image.height();
    QImage temp(w, h, image.format());

    for (int pass = 0; pass < 3; ++pass) {
        // Horizontal pass
        for (int y = 0; y < h; ++y) {
            const QRgb *src = reinterpret_cast<const QRgb *>(image.constScanLine(y));
            QRgb *dst = reinterpret_cast<QRgb *>(temp.scanLine(y));

            int rSum = 0, gSum = 0, bSum = 0, aSum = 0;
            int count = 0;

            // Initialize window
            for (int x = 0; x <= radius && x < w; ++x) {
                rSum += qRed(src[x]); gSum += qGreen(src[x]);
                bSum += qBlue(src[x]); aSum += qAlpha(src[x]);
                count++;
            }

            for (int x = 0; x < w; ++x) {
                dst[x] = qRgba(rSum / count, gSum / count, bSum / count, aSum / count);

                // Slide window
                int addX = x + radius + 1;
                int remX = x - radius;
                if (addX < w) {
                    rSum += qRed(src[addX]); gSum += qGreen(src[addX]);
                    bSum += qBlue(src[addX]); aSum += qAlpha(src[addX]);
                    count++;
                }
                if (remX >= 0) {
                    rSum -= qRed(src[remX]); gSum -= qGreen(src[remX]);
                    bSum -= qBlue(src[remX]); aSum -= qAlpha(src[remX]);
                    count--;
                }
            }
        }

        // Vertical pass
        for (int x = 0; x < w; ++x) {
            int rSum = 0, gSum = 0, bSum = 0, aSum = 0;
            int count = 0;

            for (int y = 0; y <= radius && y < h; ++y) {
                QRgb px = reinterpret_cast<const QRgb *>(temp.constScanLine(y))[x];
                rSum += qRed(px); gSum += qGreen(px);
                bSum += qBlue(px); aSum += qAlpha(px);
                count++;
            }

            for (int y = 0; y < h; ++y) {
                reinterpret_cast<QRgb *>(image.scanLine(y))[x] =
                    qRgba(rSum / count, gSum / count, bSum / count, aSum / count);

                int addY = y + radius + 1;
                int remY = y - radius;
                if (addY < h) {
                    QRgb px = reinterpret_cast<const QRgb *>(temp.constScanLine(addY))[x];
                    rSum += qRed(px); gSum += qGreen(px);
                    bSum += qBlue(px); aSum += qAlpha(px);
                    count++;
                }
                if (remY >= 0) {
                    QRgb px = reinterpret_cast<const QRgb *>(temp.constScanLine(remY))[x];
                    rSum -= qRed(px); gSum -= qGreen(px);
                    bSum -= qBlue(px); aSum -= qAlpha(px);
                    count--;
                }
            }
        }
    }
}

} // namespace screencopy
