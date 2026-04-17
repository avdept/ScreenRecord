#pragma once

#include <QAbstractListModel>
#include <QList>

namespace screencopy {

struct TrimSegment {
    qint64 startMs = 0;
    qint64 endMs = 0;
    double speed = 1.0;   // playback speed for this segment
    qint64 durationMs() const { return endMs - startMs; }
    // Effective duration accounting for speed
    qint64 effectiveDurationMs() const { return static_cast<qint64>(durationMs() / speed); }
};

class TrimModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ segmentCount NOTIFY segmentsChanged)
    Q_PROPERTY(qint64 totalDurationMs READ totalDurationMs NOTIFY segmentsChanged)
    Q_PROPERTY(qint64 originalDurationMs READ originalDurationMs NOTIFY originalDurationChanged)
    Q_PROPERTY(int selectedIndex READ selectedIndex WRITE setSelectedIndex NOTIFY selectedIndexChanged)
    Q_PROPERTY(double selectedSpeed READ selectedSpeed WRITE setSelectedSpeed NOTIFY selectedSpeedChanged)

public:
    enum Roles {
        StartMsRole = Qt::UserRole + 1,
        EndMsRole,
        DurationMsRole,
        DisplayStartMsRole,
        DisplayEndMsRole,
        SpeedRole,
        SelectedRole,
    };

    explicit TrimModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int segmentCount() const { return m_segments.size(); }
    qint64 totalDurationMs() const;
    qint64 originalDurationMs() const { return m_originalDurationMs; }

    const QList<TrimSegment> &segments() const { return m_segments; }

    // Selection
    int selectedIndex() const { return m_selectedIndex; }
    void setSelectedIndex(int index);
    double selectedSpeed() const;
    void setSelectedSpeed(double speed);
    Q_INVOKABLE void selectSegmentAt(qint64 originalMs);
    Q_INVOKABLE void clearSelection();

    // Initialize with full video duration
    Q_INVOKABLE void initialize(qint64 durationMs);

    // Split the segment at the given original-video time
    Q_INVOKABLE void splitAt(qint64 originalMs);

    // Remove a segment by index
    Q_INVOKABLE void removeSegment(int index);

    // Time mapping
    Q_INVOKABLE qint64 displayToOriginal(qint64 displayMs) const;
    Q_INVOKABLE qint64 originalToDisplay(qint64 originalMs) const;
    Q_INVOKABLE int segmentIndexAt(qint64 originalMs) const;
    Q_INVOKABLE qint64 displayStartForSegment(int index) const;
    Q_INVOKABLE qint64 segmentEndAt(qint64 originalMs) const;
    Q_INVOKABLE qint64 nextSegmentStartAfter(qint64 originalMs) const;

    // Get speed for the segment containing originalMs (1.0 if in a gap)
    Q_INVOKABLE double speedAt(qint64 originalMs) const;

    Q_INVOKABLE void reset();

signals:
    void segmentsChanged();
    void originalDurationChanged();
    void selectedIndexChanged();
    void selectedSpeedChanged();

private:
    QList<TrimSegment> m_segments;
    qint64 m_originalDurationMs = 0;
    int m_selectedIndex = -1;
};

} // namespace screencopy
