#pragma once

#include <QAbstractListModel>
#include <QVariantMap>
#include <QList>

namespace screencopy {

// Type of effect region
enum class EffectType {
    Zoom,
    Speed,
    Annotation
};

// A time-ranged effect with per-region settings.
// Each region has a type, start/end time in original video,
// and a QVariantMap of type-specific settings.
struct EffectRegion {
    int id = 0;
    EffectType type = EffectType::Zoom;
    qint64 startMs = 0;
    qint64 endMs = 0;
    QVariantMap settings;  // type-specific: {"depth": 3, "focusX": 0.5, ...}
};

// Manages all effect regions across all types.
// The timeline shows these as colored blocks on per-type tracks.
// Clicking a region selects it and shows its settings in the panel.
class EffectRegionModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ regionCount NOTIFY regionsChanged)
    Q_PROPERTY(int selectedId READ selectedId WRITE setSelectedId NOTIFY selectedIdChanged)
    Q_PROPERTY(int selectedType READ selectedType NOTIFY selectedIdChanged)
    Q_PROPERTY(QVariantMap selectedSettings READ selectedSettings NOTIFY selectedSettingsChanged)

public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        TypeRole,
        StartMsRole,
        EndMsRole,
        SettingsRole
    };

    explicit EffectRegionModel(QObject *parent = nullptr);

    // QAbstractListModel
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int regionCount() const { return m_regions.size(); }
    const QList<EffectRegion> &regions() const { return m_regions; }

    // Selection
    int selectedId() const { return m_selectedId; }
    void setSelectedId(int id);
    int selectedType() const;
    QVariantMap selectedSettings() const;

    // Add a new region. Returns the new region's ID.
    Q_INVOKABLE int addRegion(int type, qint64 startMs, qint64 endMs, const QVariantMap &settings = {});

    // Remove a region by ID
    Q_INVOKABLE void removeRegion(int id);

    // Update a region's time bounds
    Q_INVOKABLE void updateBounds(int id, qint64 startMs, qint64 endMs);

    // Update a single setting on a region
    Q_INVOKABLE void updateSetting(int id, const QString &key, const QVariant &value);

    // Update the selected region's setting (convenience)
    Q_INVOKABLE void updateSelectedSetting(const QString &key, const QVariant &value);

    // Get all regions of a specific type
    Q_INVOKABLE QVariantList regionsOfType(int type) const;

    // Get the active region of a given type at a specific time (or null)
    Q_INVOKABLE QVariantMap activeRegionAt(int type, qint64 timeMs) const;

    // Clear all regions
    Q_INVOKABLE void clear();

    // Default settings for each type
    static QVariantMap defaultSettings(EffectType type);

signals:
    void regionsChanged();
    void selectedIdChanged();
    void selectedSettingsChanged();

private:
    int indexForId(int id) const;

    QList<EffectRegion> m_regions;
    int m_nextId = 1;
    int m_selectedId = -1;
};

} // namespace screencopy
