#pragma once

#include <QObject>

namespace screencopy {

class ProjectManager;

// Global rendering settings that apply to the entire video.
// Region-specific settings (zoom depth, speed multiplier, etc.)
// live on the EffectRegion objects, not here.
class GlobalSettings : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString wallpaper READ wallpaper WRITE setWallpaper NOTIFY wallpaperChanged)
    Q_PROPERTY(double shadowIntensity READ shadowIntensity WRITE setShadowIntensity NOTIFY shadowIntensityChanged)
    Q_PROPERTY(double borderRadius READ borderRadius WRITE setBorderRadius NOTIFY borderRadiusChanged)
    Q_PROPERTY(double padding READ padding WRITE setPadding NOTIFY paddingChanged)

public:
    explicit GlobalSettings(ProjectManager *projectManager, QObject *parent = nullptr);

    QString wallpaper() const;
    void setWallpaper(const QString &value);

    double shadowIntensity() const;
    void setShadowIntensity(double value);

    double borderRadius() const;
    void setBorderRadius(double value);

    double padding() const;
    void setPadding(double value);

signals:
    void wallpaperChanged();
    void shadowIntensityChanged();
    void borderRadiusChanged();
    void paddingChanged();

private:
    ProjectManager *m_projectManager;
};

} // namespace screencopy
