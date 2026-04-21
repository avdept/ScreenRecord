#include "ShadowRenderer.h"
#include <QPainter>
#include <QPainterPath>
#include <QtMath>
#include <algorithm>

namespace screencopy {

ShadowRenderer::ShadowRenderer(QQuickItem *parent)
    : QQuickPaintedItem(parent)
{
    setRenderTarget(QQuickPaintedItem::FramebufferObject);
}

void ShadowRenderer::setShadowIntensity(double v)
{
    if (qFuzzyCompare(m_intensity, v)) return;
    m_intensity = v;
    invalidateCache();
    emit shadowIntensityChanged();
}

void ShadowRenderer::setBorderRadius(double v)
{
    if (qFuzzyCompare(m_radius, v)) return;
    m_radius = v;
    invalidateCache();
    emit borderRadiusChanged();
}

void ShadowRenderer::setVideoWidth(double v)
{
    if (qFuzzyCompare(m_videoW, v)) return;
    m_videoW = v;
    invalidateCache();
    emit videoWidthChanged();
}

void ShadowRenderer::setVideoHeight(double v)
{
    if (qFuzzyCompare(m_videoH, v)) return;
    m_videoH = v;
    invalidateCache();
    emit videoHeightChanged();
}

void ShadowRenderer::setVideoX(double v)
{
    if (qFuzzyCompare(m_videoX, v)) return;
    m_videoX = v;
    invalidateCache();
    emit videoXChanged();
}

void ShadowRenderer::setVideoY(double v)
{
    if (qFuzzyCompare(m_videoY, v)) return;
    m_videoY = v;
    invalidateCache();
    emit videoYChanged();
}

void ShadowRenderer::invalidateCache()
{
    m_dirty = true;
    update();
}

void ShadowRenderer::rebuildShadow()
{
    m_dirty = false;

    if (m_intensity <= 0.01 || m_videoW < 1 || m_videoH < 1) {
        m_shadowCache = QImage();
        return;
    }

    double si = m_intensity;

    // Exact same math as CompositeFilter::createShadow / process()
    double shadowScale = 1.0 + si * 0.05;
    int scaledW = qRound(m_videoW * shadowScale);
    int scaledH = qRound(m_videoH * shadowScale);
    double radius = m_radius * shadowScale;

    double refSize = std::min(m_videoW, m_videoH);
    int blurRadius = qRound(si * refSize * 0.035);
    int expand = blurRadius * 6;

    QSize shadowSize(scaledW + expand, scaledH + expand);
    QImage shadow(shadowSize, QImage::Format_ARGB32_Premultiplied);
    shadow.fill(Qt::transparent);

    {
        QPainter sp(&shadow);
        sp.setRenderHint(QPainter::Antialiasing);
        sp.setBrush(QColor(0, 0, 0, qRound(si * 180)));
        sp.setPen(Qt::NoPen);

        QRectF rect(expand / 2.0, expand / 2.0, scaledW, scaledH);
        if (radius > 0.1)
            sp.drawRoundedRect(rect, radius, radius);
        else
            sp.drawRect(rect);
        sp.end();
    }

    if (blurRadius > 0)
        blurImage(shadow, blurRadius);

    double shadowOffsetY = si * refSize * 0.025;

    m_dx = m_videoX - (shadow.width() - m_videoW) / 2.0;
    m_dy = m_videoY - (shadow.height() - m_videoH) / 2.0 + shadowOffsetY;

    m_shadowCache = shadow;
}

void ShadowRenderer::paint(QPainter *painter)
{
    if (m_dirty)
        rebuildShadow();

    if (m_shadowCache.isNull())
        return;

    painter->setRenderHint(QPainter::SmoothPixmapTransform);
    painter->drawImage(QPointF(m_dx, m_dy), m_shadowCache);
}

// Identical to CompositeFilter::blurImage — 3-pass box blur approximating Gaussian
void ShadowRenderer::blurImage(QImage &image, int radius)
{
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

            for (int x = 0; x <= radius && x < w; ++x) {
                rSum += qRed(src[x]); gSum += qGreen(src[x]);
                bSum += qBlue(src[x]); aSum += qAlpha(src[x]);
                count++;
            }

            for (int x = 0; x < w; ++x) {
                dst[x] = qRgba(rSum / count, gSum / count, bSum / count, aSum / count);

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
