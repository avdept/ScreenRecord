#include "ProjectManager.h"
#include <QFile>
#include <QJsonDocument>

namespace screencopy {

ProjectManager::ProjectManager(QObject *parent)
    : QObject(parent)
{
}

bool ProjectManager::saveProject(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly))
        return false;

    QJsonDocument doc(m_data.toJson());
    file.write(doc.toJson());
    file.close();

    m_currentFilePath = filePath;
    m_hasUnsavedChanges = false;
    emit currentFilePathChanged();
    emit unsavedChangesChanged();
    emit projectSaved();
    return true;
}

bool ProjectManager::loadProject(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
        return false;

    auto doc = QJsonDocument::fromJson(file.readAll());
    if (doc.isNull())
        return false;

    m_data = ProjectData::fromJson(doc.object());
    m_currentFilePath = filePath;
    m_hasUnsavedChanges = false;
    emit currentFilePathChanged();
    emit unsavedChangesChanged();
    emit projectLoaded();
    return true;
}

void ProjectManager::newProject(const QString &videoPath)
{
    m_data = ProjectData{};
    m_data.screenVideoPath = videoPath;
    m_currentFilePath.clear();
    m_hasUnsavedChanges = false;
    emit currentFilePathChanged();
    emit unsavedChangesChanged();
    emit projectLoaded();
}

void ProjectManager::markDirty()
{
    if (!m_hasUnsavedChanges) {
        m_hasUnsavedChanges = true;
        emit unsavedChangesChanged();
    }
}

} // namespace screencopy
