#include <QtTest/QtTest>
#include <QImage>
#include <QColor>

#include "CompositeFilter.h"
#include "FrameFilter.h"
#include "ProjectData.h"

using namespace screencopy;

// Measures how much "shadow" sits over a green background.
// Background is pure green (0, 255, 0). Shadow is black with alpha drawn
// over it, so result.green = 255 * (1 - alpha). Returns 0 for no shadow,
// 1 for fully opaque shadow.
static double shadowAt(const QImage &img, int x, int y)
{
    QRgb px = img.pixel(x, y);
    return 1.0 - qGreen(px) / 255.0;
}

class TstCompositeFilter : public QObject
{
    Q_OBJECT

private:
    static constexpr int kCanvasSize = 800;
    static constexpr double kPadding = 50.0;     // video is 80% of canvas
    static constexpr int kVideoSize = 640;       // derived: 800 * 0.8
    static constexpr int kVideoX = 80;           // derived: (800 - 640) / 2
    static constexpr int kVideoY = 80;

    EditorState makeEditorState(double shadowIntensity) const
    {
        EditorState ed;
        ed.wallpaper = "#00FF00";  // pure green, bypasses file IO
        ed.shadowIntensity = shadowIntensity;
        ed.padding = kPadding;
        ed.borderRadius = 0.0;
        return ed;
    }

    FilterContext makeContext(const EditorState &ed, int w, int h,
                              const QColor &videoColor = Qt::red) const
    {
        FilterContext ctx;
        ctx.outputWidth = w;
        ctx.outputHeight = h;
        ctx.editor = &ed;
        QImage frame(w, h, QImage::Format_ARGB32_Premultiplied);
        frame.fill(videoColor);
        ctx.videoFrame = frame;
        return ctx;
    }

    // Distance from video right edge at which shadow alpha drops below threshold.
    // Walks right from video edge along horizontal center line.
    int shadowReachRight(const QImage &img, int videoRight, int centerY,
                         double threshold = 0.02) const
    {
        for (int dx = 1; videoRight + dx < img.width(); ++dx) {
            if (shadowAt(img, videoRight + dx, centerY) < threshold)
                return dx;
        }
        return img.width() - videoRight;
    }

private slots:
    void testNoShadowWhenIntensityZero()
    {
        CompositeFilter filter;
        EditorState ed = makeEditorState(0.0);
        FilterContext ctx = makeContext(ed, kCanvasSize, kCanvasSize);

        QImage out = filter.process(ctx.videoFrame, ctx);

        // Sample pixels in the padding region (outside video rect).
        // All should be pure green — no shadow contribution.
        const QList<QPoint> paddingPoints = {
            {10, 10}, {10, 400}, {400, 10},
            {790, 790}, {790, 400}, {400, 790}
        };
        for (const QPoint &p : paddingPoints) {
            QRgb px = out.pixel(p);
            QVERIFY2(qRed(px) == 0 && qGreen(px) == 255 && qBlue(px) == 0,
                     qPrintable(QString("pixel at (%1,%2) not pure green: %3,%4,%5")
                                .arg(p.x()).arg(p.y())
                                .arg(qRed(px)).arg(qGreen(px)).arg(qBlue(px))));
        }
    }

    // Regression test for the "semi-transparent rectangle" bug:
    // shadow was being clipped at the shadow canvas boundary, leaving
    // a visible hard rectangular edge instead of a soft fade.
    void testShadowFadesSmoothlyToBackground()
    {
        CompositeFilter filter;
        EditorState ed = makeEditorState(0.8);
        FilterContext ctx = makeContext(ed, kCanvasSize, kCanvasSize);

        QImage out = filter.process(ctx.videoFrame, ctx);

        // Walk rightward from the video edge along the horizontal midline.
        // Shadow darkness should be monotonically non-increasing — any upward
        // jump would indicate a hard edge (buggy clipping).
        const int videoRight = kVideoX + kVideoSize;   // 720
        const int cy = kCanvasSize / 2;                // 400

        double prev = shadowAt(out, videoRight + 1, cy);
        QVERIFY2(prev > 0.05, "expected visible shadow just outside video");

        double maxJump = 0.0;
        for (int dx = 2; videoRight + dx < kCanvasSize; ++dx) {
            double s = shadowAt(out, videoRight + dx, cy);
            // Allow small positive jumps for sampling noise but none should be large.
            if (s - prev > maxJump)
                maxJump = s - prev;
            prev = s;
        }
        QVERIFY2(maxJump < 0.02,
                 qPrintable(QString("shadow alpha jumped by %1 — hard edge?").arg(maxJump)));

        // Shadow should eventually fade to near-background near the canvas corner.
        QVERIFY2(shadowAt(out, kCanvasSize - 1, kCanvasSize - 1) < 0.05,
                 "shadow still present at far canvas corner — not fading properly");
    }

    void testShadowOffsetDownward()
    {
        CompositeFilter filter;
        EditorState ed = makeEditorState(0.8);
        FilterContext ctx = makeContext(ed, kCanvasSize, kCanvasSize);

        QImage out = filter.process(ctx.videoFrame, ctx);

        // Sample same distance above video top and below video bottom, at horizontal center.
        const int cx = kCanvasSize / 2;              // 400
        const int videoTop = kVideoY;                // 80
        const int videoBottom = kVideoY + kVideoSize; // 720
        const int d = 15;

        double above = shadowAt(out, cx, videoTop - d);
        double below = shadowAt(out, cx, videoBottom + d);

        QVERIFY2(below > above,
                 qPrintable(QString("expected shadow below video (%1) > above (%2)")
                            .arg(below).arg(above)));
    }

    // When output resolution doubles, shadow blur/offset should roughly double too.
    // Without resolution scaling, the shadow would look proportionally smaller at higher res.
    void testBlurScalesWithResolution()
    {
        const double si = 0.8;
        const int smallW = 400;
        const int largeW = 800;

        CompositeFilter filter;
        EditorState ed = makeEditorState(si);

        FilterContext ctxSmall = makeContext(ed, smallW, smallW);
        FilterContext ctxLarge = makeContext(ed, largeW, largeW);

        QImage outSmall = filter.process(ctxSmall.videoFrame, ctxSmall);
        QImage outLarge = filter.process(ctxLarge.videoFrame, ctxLarge);

        // Video rect is 80% of canvas, centered.
        const int smallVideoRight = smallW - int(smallW * 0.1);   // 360
        const int largeVideoRight = largeW - int(largeW * 0.1);   // 720

        int reachSmall = shadowReachRight(outSmall, smallVideoRight, smallW / 2);
        int reachLarge = shadowReachRight(outLarge, largeVideoRight, largeW / 2);

        double ratio = double(reachLarge) / double(reachSmall);
        QVERIFY2(ratio > 1.7 && ratio < 2.3,
                 qPrintable(QString("blur reach ratio %1 (large=%2, small=%3) — expected ~2.0")
                            .arg(ratio).arg(reachLarge).arg(reachSmall)));
    }

    void testVideoFrameRendered()
    {
        CompositeFilter filter;
        EditorState ed = makeEditorState(0.0);  // no shadow to not pollute
        FilterContext ctx = makeContext(ed, kCanvasSize, kCanvasSize, Qt::red);

        QImage out = filter.process(ctx.videoFrame, ctx);

        // Canvas center is inside video rect — should be pure red.
        QRgb px = out.pixel(kCanvasSize / 2, kCanvasSize / 2);
        QCOMPARE(qRed(px), 255);
        QCOMPARE(qGreen(px), 0);
        QCOMPARE(qBlue(px), 0);
    }
};

QTEST_MAIN(TstCompositeFilter)
#include "tst_compositefilter.moc"
