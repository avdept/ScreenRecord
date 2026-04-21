#pragma once

#include <QQuickPaintedItem>
#include <QImage>

namespace screencopy {

class ShadowRenderer : public QQuickPaintedItem
{
    Q_OBJECT
    Q_PROPERTY(double shadowIntensity READ shadowIntensity WRITE setShadowIntensity NOTIFY shadowIntensityChanged)
    Q_PROPERTY(double borderRadius READ borderRadius WRITE setBorderRadius NOTIFY borderRadiusChanged)
    Q_PROPERTY(double videoWidth READ videoWidth WRITE setVideoWidth NOTIFY videoWidthChanged)
    Q_PROPERTY(double videoHeight READ videoHeight WRITE setVideoHeight NOTIFY videoHeightChanged)
    Q_PROPERTY(double videoX READ videoX WRITE setVideoX NOTIFY videoXChanged)
    Q_PROPERTY(double videoY READ videoY WRITE setVideoY NOTIFY videoYChanged)

public:
    explicit ShadowRenderer(QQuickItem *parent = nullptr);

    double shadowIntensity() const { return m_intensity; }
    void setShadowIntensity(double v);

    double borderRadius() const { return m_radius; }
    void setBorderRadius(double v);

    double videoWidth() const { return m_videoW; }
    void setVideoWidth(double v);

    double videoHeight() const { return m_videoH; }
    void setVideoHeight(double v);

    double videoX() const { return m_videoX; }
    void setVideoX(double v);

    double videoY() const { return m_videoY; }
    void setVideoY(double v);

    void paint(QPainter *painter) override;

signals:
    void shadowIntensityChanged();
    void borderRadiusChanged();
    void videoWidthChanged();
    void videoHeightChanged();
    void videoXChanged();
    void videoYChanged();

private:
    void invalidateCache();
    void rebuildShadow();
    static void blurImage(QImage &image, int radius);

    double m_intensity = 0;
    double m_radius = 0;
    double m_videoW = 0;
    double m_videoH = 0;
    double m_videoX = 0;
    double m_videoY = 0;

    bool m_dirty = true;
    QImage m_shadowCache;
    // Cached layout values for paint()
    double m_dx = 0;
    double m_dy = 0;
};

} // namespace screencopy
