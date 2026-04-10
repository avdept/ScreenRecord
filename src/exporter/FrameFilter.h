#pragma once

#include <QImage>
#include <QString>
#include <QVariantMap>
#include <QList>
#include <memory>

namespace screencopy {

struct EditorState;

// Active effect region at the current frame time
struct ActiveEffect {
    int type = 0;          // EffectType enum value
    qint64 startMs = 0;
    qint64 endMs = 0;
    QVariantMap settings;  // per-region settings
};

// Context passed to each filter for the current frame.
// Carries everything a filter needs: global settings, raw frame,
// and any active effect regions at this timestamp.
struct FilterContext {
    qint64 timeMs = 0;              // Current frame timestamp
    int outputWidth = 1920;         // Final output dimensions
    int outputHeight = 1080;
    QImage videoFrame;               // Raw decoded video frame
    const EditorState *editor = nullptr;  // Global settings
    QList<ActiveEffect> activeEffects;    // Effect regions active at timeMs
};

// Abstract filter interface — each visual effect implements this.
class FrameFilter
{
public:
    virtual ~FrameFilter() = default;

    virtual QImage process(const QImage &input, const FilterContext &ctx) = 0;
    virtual QString name() const = 0;
    virtual bool isEnabled(const FilterContext &ctx) const { Q_UNUSED(ctx); return true; }
};

using FrameFilterPtr = std::unique_ptr<FrameFilter>;

} // namespace screencopy
