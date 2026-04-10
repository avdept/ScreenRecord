#include "FilterPipeline.h"
#include <QDebug>

namespace screencopy {

void FilterPipeline::addFilter(FrameFilterPtr filter)
{
    m_filters.push_back(std::move(filter));
}

void FilterPipeline::clear()
{
    m_filters.clear();
}

QImage FilterPipeline::process(const QImage &input, const FilterContext &ctx) const
{
    QImage current = input;

    for (const auto &filter : m_filters) {
        if (filter->isEnabled(ctx)) {
            current = filter->process(current, ctx);
        }
    }

    return current;
}

QStringList FilterPipeline::filterNames() const
{
    QStringList names;
    for (const auto &f : m_filters)
        names << f->name();
    return names;
}

} // namespace screencopy
