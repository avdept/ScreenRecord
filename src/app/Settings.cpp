#include "Settings.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QDir>
#include <QFile>
#include <QStandardPaths>

namespace screencopy {

Settings::Settings(QObject *parent)
    : QObject(parent)
    , m_settings("ScreenCopy", "ScreenCopy")
{
}

double Settings::defaultPadding() const { return m_settings.value("defaultPadding", 10.0).toDouble(); }
void Settings::setDefaultPadding(double value) { m_settings.setValue("defaultPadding", value); emit defaultPaddingChanged(); }

QString Settings::defaultAspectRatio() const { return m_settings.value("defaultAspectRatio", "16:9").toString(); }
void Settings::setDefaultAspectRatio(const QString &value) { m_settings.setValue("defaultAspectRatio", value); emit defaultAspectRatioChanged(); }

QString Settings::defaultExportQuality() const { return m_settings.value("defaultExportQuality", "good").toString(); }
void Settings::setDefaultExportQuality(const QString &value) { m_settings.setValue("defaultExportQuality", value); emit defaultExportQualityChanged(); }

QString Settings::defaultExportFormat() const { return m_settings.value("defaultExportFormat", "mp4").toString(); }
void Settings::setDefaultExportFormat(const QString &value) { m_settings.setValue("defaultExportFormat", value); emit defaultExportFormatChanged(); }

QVariantMap Settings::loadShortcuts()
{
    auto dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QFile file(dataDir + "/shortcuts.json");
    if (!file.open(QIODevice::ReadOnly))
        return {};

    auto doc = QJsonDocument::fromJson(file.readAll());
    return doc.object().toVariantMap();
}

void Settings::saveShortcuts(const QVariantMap &shortcuts)
{
    auto dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataDir);
    QFile file(dataDir + "/shortcuts.json");
    if (!file.open(QIODevice::WriteOnly))
        return;

    file.write(QJsonDocument(QJsonObject::fromVariantMap(shortcuts)).toJson());
}

} // namespace screencopy
