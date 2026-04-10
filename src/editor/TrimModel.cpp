#include "TrimModel.h"

namespace screencopy {

TrimModel::TrimModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int TrimModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_segments.size();
}

QVariant TrimModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_segments.size())
        return {};

    const auto &seg = m_segments[index.row()];
    switch (role) {
    case StartMsRole:        return seg.startMs;
    case EndMsRole:          return seg.endMs;
    case DurationMsRole:     return seg.durationMs();
    case DisplayStartMsRole: return displayStartForSegment(index.row());
    case DisplayEndMsRole:   return displayStartForSegment(index.row()) + seg.durationMs();
    default: return {};
    }
}

QHash<int, QByteArray> TrimModel::roleNames() const
{
    return {
        {StartMsRole, "startMs"},
        {EndMsRole, "endMs"},
        {DurationMsRole, "durationMs"},
        {DisplayStartMsRole, "displayStartMs"},
        {DisplayEndMsRole, "displayEndMs"},
    };
}

qint64 TrimModel::totalDurationMs() const
{
    qint64 total = 0;
    for (const auto &seg : m_segments)
        total += seg.durationMs();
    return total;
}

void TrimModel::initialize(qint64 durationMs)
{
    beginResetModel();
    m_segments.clear();
    m_originalDurationMs = durationMs;
    if (durationMs > 0)
        m_segments.append({0, durationMs});
    endResetModel();
    emit segmentsChanged();
    emit originalDurationChanged();
}

void TrimModel::splitAt(qint64 originalMs)
{
    // Find the segment that contains this time
    int idx = segmentIndexAt(originalMs);
    if (idx < 0)
        return;

    const auto &seg = m_segments[idx];

    // Don't split at the very start or end of a segment (< 100ms from edge)
    if (originalMs - seg.startMs < 100 || seg.endMs - originalMs < 100)
        return;

    // Split into two segments
    TrimSegment left{seg.startMs, originalMs};
    TrimSegment right{originalMs, seg.endMs};

    beginRemoveRows(QModelIndex(), idx, idx);
    m_segments.removeAt(idx);
    endRemoveRows();

    beginInsertRows(QModelIndex(), idx, idx + 1);
    m_segments.insert(idx, right);
    m_segments.insert(idx, left);
    endInsertRows();

    // All segments' display positions may have shifted
    if (m_segments.size() > 0) {
        emit dataChanged(this->index(0), this->index(m_segments.size() - 1),
                         {DisplayStartMsRole, DisplayEndMsRole});
    }

    emit segmentsChanged();
}

void TrimModel::removeSegment(int index)
{
    if (index < 0 || index >= m_segments.size())
        return;

    // Don't remove the last segment
    if (m_segments.size() <= 1)
        return;

    beginRemoveRows(QModelIndex(), index, index);
    m_segments.removeAt(index);
    endRemoveRows();

    // All remaining segments have shifted display positions — notify QML
    if (!m_segments.isEmpty()) {
        emit dataChanged(this->index(0), this->index(m_segments.size() - 1),
                         {DisplayStartMsRole, DisplayEndMsRole});
    }

    emit segmentsChanged();
}

qint64 TrimModel::displayToOriginal(qint64 displayMs) const
{
    qint64 accumulated = 0;
    for (const auto &seg : m_segments) {
        qint64 segDur = seg.durationMs();
        if (displayMs <= accumulated + segDur) {
            // It's within this segment
            return seg.startMs + (displayMs - accumulated);
        }
        accumulated += segDur;
    }
    // Past the end — return end of last segment
    if (!m_segments.isEmpty())
        return m_segments.last().endMs;
    return 0;
}

qint64 TrimModel::originalToDisplay(qint64 originalMs) const
{
    qint64 displayMs = 0;
    for (const auto &seg : m_segments) {
        if (originalMs < seg.startMs) {
            // In a gap before this segment — clamp to start of this segment
            return displayMs;
        }
        if (originalMs <= seg.endMs) {
            // Within this segment
            return displayMs + (originalMs - seg.startMs);
        }
        displayMs += seg.durationMs();
    }
    return displayMs;
}

qint64 TrimModel::segmentEndAt(qint64 originalMs) const
{
    for (const auto &seg : m_segments) {
        if (originalMs >= seg.startMs && originalMs <= seg.endMs)
            return seg.endMs;
    }
    return -1;
}

int TrimModel::segmentIndexAt(qint64 originalMs) const
{
    for (int i = 0; i < m_segments.size(); ++i) {
        if (originalMs >= m_segments[i].startMs && originalMs <= m_segments[i].endMs)
            return i;
    }
    return -1;
}

qint64 TrimModel::displayStartForSegment(int index) const
{
    qint64 pos = 0;
    for (int i = 0; i < index && i < m_segments.size(); ++i)
        pos += m_segments[i].durationMs();
    return pos;
}

qint64 TrimModel::nextSegmentStartAfter(qint64 originalMs) const
{
    for (const auto &seg : m_segments) {
        if (seg.startMs > originalMs)
            return seg.startMs;
    }
    return -1;
}

void TrimModel::reset()
{
    initialize(m_originalDurationMs);
}

} // namespace screencopy
