#pragma once

#include "FrameFilter.h"
#include <QImage>
#include <QCache>

namespace screencopy {

// Composites the video frame onto a background with:
// 1. Wallpaper (image or solid color) as background
// 2. Padding (video scaled and centered)
// 3. Border radius (rounded corners on the video)
// 4. Shadow (Gaussian-blurred drop shadow behind the video)
//
// These are combined into one filter because they're spatially interdependent —
// the shadow shape depends on border radius, the video position depends on padding,
// and the background fills the area around the padded+rounded video.
class CompositeFilter : public FrameFilter
{
public:
    CompositeFilter();

    QImage process(const QImage &input, const FilterContext &ctx) override;
    QString name() const override { return "Composite"; }

private:
    QImage loadWallpaper(const QString &wpName, const QSize &size);
    QImage createShadow(const QSize &videoSize, double radius, double intensity);
    void blurImage(QImage &image, int radius);

    QCache<QString, QImage> m_wallpaperCache;
};

} // namespace screencopy
