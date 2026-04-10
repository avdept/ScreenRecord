#pragma once

#include <QObject>
#include "ProjectData.h"

namespace screencopy {

class ProjectManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool hasUnsavedChanges READ hasUnsavedChanges NOTIFY unsavedChangesChanged)
    Q_PROPERTY(QString currentFilePath READ currentFilePath NOTIFY currentFilePathChanged)

public:
    explicit ProjectManager(QObject *parent = nullptr);

    bool hasUnsavedChanges() const { return m_hasUnsavedChanges; }
    QString currentFilePath() const { return m_currentFilePath; }

    const ProjectData &projectData() const { return m_data; }
    EditorState &editorState() { return m_data.editor; }

    Q_INVOKABLE bool saveProject(const QString &filePath);
    Q_INVOKABLE bool loadProject(const QString &filePath);
    Q_INVOKABLE void newProject(const QString &videoPath);
    Q_INVOKABLE void markDirty();

signals:
    void projectLoaded();
    void projectSaved();
    void unsavedChangesChanged();
    void currentFilePathChanged();

private:
    ProjectData m_data;
    QString m_currentFilePath;
    bool m_hasUnsavedChanges = false;
};

} // namespace screencopy
