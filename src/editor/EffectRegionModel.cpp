#include "EffectRegionModel.h"

namespace screencopy {

EffectRegionModel::EffectRegionModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int EffectRegionModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_regions.size();
}

QVariant EffectRegionModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_regions.size())
        return {};

    const auto &r = m_regions[index.row()];
    switch (role) {
    case IdRole:       return r.id;
    case TypeRole:     return static_cast<int>(r.type);
    case StartMsRole:  return r.startMs;
    case EndMsRole:    return r.endMs;
    case SettingsRole: return r.settings;
    default: return {};
    }
}

QHash<int, QByteArray> EffectRegionModel::roleNames() const
{
    return {
        {IdRole, "regionId"},
        {TypeRole, "regionType"},
        {StartMsRole, "startMs"},
        {EndMsRole, "endMs"},
        {SettingsRole, "settings"},
    };
}

void EffectRegionModel::setSelectedId(int id)
{
    if (m_selectedId != id) {
        m_selectedId = id;
        emit selectedIdChanged();
        emit selectedSettingsChanged();
    }
}

int EffectRegionModel::selectedType() const
{
    int idx = indexForId(m_selectedId);
    if (idx < 0) return -1;
    return static_cast<int>(m_regions[idx].type);
}

QVariantMap EffectRegionModel::selectedSettings() const
{
    int idx = indexForId(m_selectedId);
    if (idx < 0) return {};
    return m_regions[idx].settings;
}

int EffectRegionModel::addRegion(int type, qint64 startMs, qint64 endMs, const QVariantMap &settings)
{
    EffectRegion r;
    r.id = m_nextId++;
    r.type = static_cast<EffectType>(type);
    r.startMs = startMs;
    r.endMs = endMs;
    r.settings = settings.isEmpty() ? defaultSettings(r.type) : settings;

    int row = m_regions.size();
    beginInsertRows(QModelIndex(), row, row);
    m_regions.append(r);
    endInsertRows();

    emit regionsChanged();
    return r.id;
}

void EffectRegionModel::removeRegion(int id)
{
    int idx = indexForId(id);
    if (idx < 0) return;

    beginRemoveRows(QModelIndex(), idx, idx);
    m_regions.removeAt(idx);
    endRemoveRows();

    if (m_selectedId == id)
        setSelectedId(-1);

    emit regionsChanged();
}

void EffectRegionModel::updateBounds(int id, qint64 startMs, qint64 endMs)
{
    int idx = indexForId(id);
    if (idx < 0) return;

    m_regions[idx].startMs = startMs;
    m_regions[idx].endMs = endMs;
    emit dataChanged(index(idx), index(idx), {StartMsRole, EndMsRole});
}

void EffectRegionModel::updateSetting(int id, const QString &key, const QVariant &value)
{
    int idx = indexForId(id);
    if (idx < 0) return;

    m_regions[idx].settings[key] = value;
    emit dataChanged(index(idx), index(idx), {SettingsRole});

    if (id == m_selectedId)
        emit selectedSettingsChanged();
}

void EffectRegionModel::updateSelectedSetting(const QString &key, const QVariant &value)
{
    if (m_selectedId >= 0)
        updateSetting(m_selectedId, key, value);
}

QVariantList EffectRegionModel::regionsOfType(int type) const
{
    QVariantList result;
    for (const auto &r : m_regions) {
        if (static_cast<int>(r.type) == type) {
            QVariantMap m;
            m["id"] = r.id;
            m["startMs"] = r.startMs;
            m["endMs"] = r.endMs;
            m["settings"] = r.settings;
            result.append(m);
        }
    }
    return result;
}

QVariantMap EffectRegionModel::activeRegionAt(int type, qint64 timeMs) const
{
    for (const auto &r : m_regions) {
        if (static_cast<int>(r.type) == type && timeMs >= r.startMs && timeMs <= r.endMs) {
            QVariantMap m;
            m["id"] = r.id;
            m["startMs"] = r.startMs;
            m["endMs"] = r.endMs;
            m["settings"] = r.settings;
            return m;
        }
    }
    return {};
}

void EffectRegionModel::clear()
{
    beginResetModel();
    m_regions.clear();
    m_selectedId = -1;
    endResetModel();
    emit regionsChanged();
    emit selectedIdChanged();
}

QVariantMap EffectRegionModel::defaultSettings(EffectType type)
{
    switch (type) {
    case EffectType::Zoom:
        return {
            {"depth", 2},
            {"focusX", 0.5},
            {"focusY", 0.5},
            {"autoFollow", true}
        };
    case EffectType::Speed:
        return {
            {"speed", 1.5}
        };
    case EffectType::Annotation:
        return {
            {"text", ""},
            {"fontFamily", "system-ui"},
            {"fontSize", 24},
            {"color", "#ffffff"},
            {"x", 0.1},
            {"y", 0.1},
            {"width", 0.3},
            {"height", 0.1}
        };
    }
    return {};
}

int EffectRegionModel::indexForId(int id) const
{
    for (int i = 0; i < m_regions.size(); ++i) {
        if (m_regions[i].id == id)
            return i;
    }
    return -1;
}

} // namespace screencopy
