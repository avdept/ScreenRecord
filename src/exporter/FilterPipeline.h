#pragma once

#include "FrameFilter.h"
#include <vector>

namespace screencopy {

// Chains multiple FrameFilters in order.
// Usage:
//   pipeline.addFilter(std::make_unique<WallpaperFilter>());
//   pipeline.addFilter(std::make_unique<PaddingFilter>());
//   pipeline.addFilter(std::make_unique<ShadowFilter>());
//   pipeline.addFilter(std::make_unique<BorderRadiusFilter>());
//   QImage result = pipeline.process(rawFrame, ctx);
class FilterPipeline
{
public:
    FilterPipeline() = default;

    void addFilter(FrameFilterPtr filter);
    void clear();
    int filterCount() const { return static_cast<int>(m_filters.size()); }

    // Run the frame through all filters in order.
    // Skips filters where isEnabled() returns false.
    QImage process(const QImage &input, const FilterContext &ctx) const;

    // Get filter names (for debug/logging)
    QStringList filterNames() const;

private:
    std::vector<FrameFilterPtr> m_filters;
};

} // namespace screencopy
