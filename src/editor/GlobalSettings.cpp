#include "GlobalSettings.h"
#include "ProjectManager.h"

namespace screencopy {

GlobalSettings::GlobalSettings(ProjectManager *projectManager, QObject *parent)
    : QObject(parent)
    , m_projectManager(projectManager)
{
}

QString GlobalSettings::wallpaper() const { return m_projectManager->editorState().wallpaper; }
void GlobalSettings::setWallpaper(const QString &v) { m_projectManager->editorState().wallpaper = v; m_projectManager->markDirty(); emit wallpaperChanged(); }

double GlobalSettings::shadowIntensity() const { return m_projectManager->editorState().shadowIntensity; }
void GlobalSettings::setShadowIntensity(double v) { m_projectManager->editorState().shadowIntensity = v; m_projectManager->markDirty(); emit shadowIntensityChanged(); }

double GlobalSettings::borderRadius() const { return m_projectManager->editorState().borderRadius; }
void GlobalSettings::setBorderRadius(double v) { m_projectManager->editorState().borderRadius = v; m_projectManager->markDirty(); emit borderRadiusChanged(); }

double GlobalSettings::padding() const { return m_projectManager->editorState().padding; }
void GlobalSettings::setPadding(double v) { m_projectManager->editorState().padding = v; m_projectManager->markDirty(); emit paddingChanged(); }

} // namespace screencopy
