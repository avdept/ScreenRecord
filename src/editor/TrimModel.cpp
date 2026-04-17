#include "TrimModel.h"
#include "ProjectTypes.h"

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
    case DisplayEndMsRole:   return displayStartForSegment(index.row()) + seg.effectiveDurationMs();
    case SpeedRole:          return seg.speed;
    case SelectedRole:       return index.row() == m_selectedIndex;
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
        {SpeedRole, "speed"},
        {SelectedRole, "selected"},
    };
}

qint64 TrimModel::totalDurationMs() const
{
    qint64 total = 0;
    for (const auto &seg : m_segments)
        total += seg.effectiveDurationMs();
    return total;
}

void TrimModel::setSelectedIndex(int index)
{
    if (m_selectedIndex == index) return;

    int oldIdx = m_selectedIndex;
    m_selectedIndex = index;

    // Notify old and new rows about selection change
    if (oldIdx >= 0 && oldIdx < m_segments.size())
        emit dataChanged(this->index(oldIdx), this->index(oldIdx), {SelectedRole});
    if (index >= 0 && index < m_segments.size())
        emit dataChanged(this->index(index), this->index(index), {SelectedRole});

    emit selectedIndexChanged();
    emit selectedSpeedChanged();
}

double TrimModel::selectedSpeed() const
{
    if (m_selectedIndex < 0 || m_selectedIndex >= m_segments.size())
        return 1.0;
    return m_segments[m_selectedIndex].speed;
}

void TrimModel::setSelectedSpeed(double speed)
{
    if (m_selectedIndex < 0 || m_selectedIndex >= m_segments.size())
        return;

    speed = qBound(kMinPlaybackSpeed, speed, kMaxPlaybackSpeed);
    if (qFuzzyCompare(m_segments[m_selectedIndex].speed, speed))
        return;

    m_segments[m_selectedIndex].speed = speed;

    // Speed change affects display positions of this and all subsequent segments
    if (!m_segments.isEmpty()) {
        emit dataChanged(this->index(0), this->index(m_segments.size() - 1),
                         {SpeedRole, DisplayStartMsRole, DisplayEndMsRole});
    }

    emit selectedSpeedChanged();
    emit segmentsChanged();
}

void TrimModel::selectSegmentAt(qint64 originalMs)
{
    setSelectedIndex(segmentIndexAt(originalMs));
}

void TrimModel::clearSelection()
{
    setSelectedIndex(-1);
}

void TrimModel::initialize(qint64 durationMs)
{
    beginResetModel();
    m_segments.clear();
    m_originalDurationMs = durationMs;
    m_selectedIndex = -1;
    if (durationMs > 0)
        m_segments.append({0, durationMs, 1.0});
    endResetModel();
    emit segmentsChanged();
    emit originalDurationChanged();
    emit selectedIndexChanged();
}

void TrimModel::splitAt(qint64 originalMs)
{
    int idx = segmentIndexAt(originalMs);
    if (idx < 0) return;

    const auto &seg = m_segments[idx];
    if (originalMs - seg.startMs < 100 || seg.endMs - originalMs < 100)
        return;

    double origSpeed = seg.speed;
    TrimSegment left{seg.startMs, originalMs, origSpeed};
    TrimSegment right{originalMs, seg.endMs, origSpeed};

    beginRemoveRows(QModelIndex(), idx, idx);
    m_segments.removeAt(idx);
    endRemoveRows();

    beginInsertRows(QModelIndex(), idx, idx + 1);
    m_segments.insert(idx, right);
    m_segments.insert(idx, left);
    endInsertRows();

    if (m_segments.size() > 0) {
        emit dataChanged(this->index(0), this->index(m_segments.size() - 1),
                         {DisplayStartMsRole, DisplayEndMsRole});
    }

    // Clear selection after split
    clearSelection();
    emit segmentsChanged();
}

void TrimModel::removeSegment(int index)
{
    if (index < 0 || index >= m_segments.size()) return;
    if (m_segments.size() <= 1) return;

    if (m_selectedIndex == index)
        clearSelection();
    else if (m_selectedIndex > index)
        m_selectedIndex--;

    beginRemoveRows(QModelIndex(), index, index);
    m_segments.removeAt(index);
    endRemoveRows();

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
        qint64 effDur = seg.effectiveDurationMs();
        if (displayMs <= accumulated + effDur) {
            // Convert display offset within this segment back to original time
            // displayOffset / effectiveDuration * originalDuration = originalOffset
            qint64 displayOffset = displayMs - accumulated;
            qint64 originalOffset = static_cast<qint64>(displayOffset * seg.speed);
            return seg.startMs + originalOffset;
        }
        accumulated += effDur;
    }
    if (!m_segments.isEmpty())
        return m_segments.last().endMs;
    return 0;
}

qint64 TrimModel::originalToDisplay(qint64 originalMs) const
{
    qint64 displayMs = 0;
    for (const auto &seg : m_segments) {
        if (originalMs < seg.startMs)
            return displayMs;
        if (originalMs <= seg.endMs) {
            // Convert original offset to display offset accounting for speed
            qint64 originalOffset = originalMs - seg.startMs;
            qint64 displayOffset = static_cast<qint64>(originalOffset / seg.speed);
            return displayMs + displayOffset;
        }
        displayMs += seg.effectiveDurationMs();
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
        pos += m_segments[i].effectiveDurationMs();
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

double TrimModel::speedAt(qint64 originalMs) const
{
    int idx = segmentIndexAt(originalMs);
    if (idx < 0) return 1.0;
    return m_segments[idx].speed;
}

void TrimModel::reset()
{
    initialize(m_originalDurationMs);
}

} // namespace screencopy
