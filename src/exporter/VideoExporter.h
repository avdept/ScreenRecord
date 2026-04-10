#pragma once

#include <QObject>
#include <QThread>
#include <atomic>
#include "ExportTypes.h"
#include "FilterPipeline.h"
#include "ProjectData.h"
#include "TrimModel.h"
#include "EffectRegionModel.h"

namespace screencopy {

class FFmpegDecoder;
class FFmpegEncoder;
class ProjectManager;
class TrimModel;
class EffectRegionModel;

class VideoExporter : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool exporting READ isExporting NOTIFY exportingChanged)
    Q_PROPERTY(double progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)

public:
    explicit VideoExporter(QObject *parent = nullptr);

    void setProjectManager(ProjectManager *pm) { m_projectManager = pm; }
    void setTrimModel(TrimModel *tm) { m_trimModel = tm; }
    void setEffectRegions(EffectRegionModel *er) { m_effectRegions = er; }

    bool isExporting() const { return m_exporting; }
    double progress() const { return m_progress; }
    QString statusText() const { return m_statusText; }

    static FilterPipeline createDefaultPipeline();

    // QML-friendly: reads EditorState from ProjectManager
    Q_INVOKABLE void startExport(const QString &inputPath,
                                 const QString &outputPath,
                                 int qualityIndex);

    // Opens native file dialog, then starts export
    Q_INVOKABLE void startExportWithDialog(const QString &inputPath, int qualityIndex);

    Q_INVOKABLE void cancelExport();

signals:
    void exportingChanged();
    void progressChanged();
    void statusTextChanged();
    void exportFinished(const QString &outputPath);
    void exportError(const QString &error);

private:
    void runExport(const QString &inputPath,
                   const QString &outputPath,
                   const EditorState &editorState,
                   ExportQuality quality,
                   const QList<TrimSegment> &segments,
                   const QList<EffectRegion> &effectRegions);

    ProjectManager *m_projectManager = nullptr;
    TrimModel *m_trimModel = nullptr;
    EffectRegionModel *m_effectRegions = nullptr;
    std::atomic<bool> m_cancelled{false};
    bool m_exporting = false;
    double m_progress = 0.0;
    QString m_statusText;
    QThread *m_workerThread = nullptr;
};

} // namespace screencopy
