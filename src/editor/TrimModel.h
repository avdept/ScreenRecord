#pragma once

#include <QAbstractListModel>
#include <QList>

namespace screencopy {

struct TrimSegment {
    qint64 startMs = 0;   // start in original video time
    qint64 endMs = 0;     // end in original video time
    qint64 durationMs() const { return endMs - startMs; }
};

// Non-destructive trim model.
// Maintains a list of "keep" segments in original video time.
// Gaps between segments = cut parts that won't appear in output.
//
// Initially: one segment [0, videoDuration].
// splitAt(ms): splits the segment containing ms into two.
// removeSegment(index): deletes a segment (marks that region as cut).
// The display timeline collapses gaps — segments appear back-to-back.
class TrimModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ segmentCount NOTIFY segmentsChanged)
    Q_PROPERTY(qint64 totalDurationMs READ totalDurationMs NOTIFY segmentsChanged)
    Q_PROPERTY(qint64 originalDurationMs READ originalDurationMs NOTIFY originalDurationChanged)

public:
    enum Roles {
        StartMsRole = Qt::UserRole + 1,
        EndMsRole,
        DurationMsRole,
        // Position in the collapsed (display) timeline
        DisplayStartMsRole,
        DisplayEndMsRole,
    };

    explicit TrimModel(QObject *parent = nullptr);

    // QAbstractListModel
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int segmentCount() const { return m_segments.size(); }
    qint64 totalDurationMs() const;
    qint64 originalDurationMs() const { return m_originalDurationMs; }

    const QList<TrimSegment> &segments() const { return m_segments; }

    // Initialize with full video duration
    Q_INVOKABLE void initialize(qint64 durationMs);

    // Split the segment at the given original-video time
    Q_INVOKABLE void splitAt(qint64 originalMs);

    // Remove a segment by index
    Q_INVOKABLE void removeSegment(int index);

    // Map display time (collapsed, no gaps) → original video time
    Q_INVOKABLE qint64 displayToOriginal(qint64 displayMs) const;

    // Map original video time → display time
    Q_INVOKABLE qint64 originalToDisplay(qint64 originalMs) const;

    // Find which segment contains the given original time (-1 if in a gap)
    Q_INVOKABLE int segmentIndexAt(qint64 originalMs) const;

    // Get display-timeline start position for a segment
    Q_INVOKABLE qint64 displayStartForSegment(int index) const;

    // Get the end time of the segment containing originalMs. Returns -1 if in a gap.
    Q_INVOKABLE qint64 segmentEndAt(qint64 originalMs) const;

    // Get the start of the next kept segment after the given original time.
    // Returns -1 if there is no next segment.
    Q_INVOKABLE qint64 nextSegmentStartAfter(qint64 originalMs) const;

    // Reset to single segment covering full duration
    Q_INVOKABLE void reset();

signals:
    void segmentsChanged();
    void originalDurationChanged();

private:
    QList<TrimSegment> m_segments;
    qint64 m_originalDurationMs = 0;
};

} // namespace screencopy
