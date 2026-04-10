#pragma once

#include <QObject>
#include <QSettings>

namespace screencopy {

class Settings : public QObject
{
    Q_OBJECT
    Q_PROPERTY(double defaultPadding READ defaultPadding WRITE setDefaultPadding NOTIFY defaultPaddingChanged)
    Q_PROPERTY(QString defaultAspectRatio READ defaultAspectRatio WRITE setDefaultAspectRatio NOTIFY defaultAspectRatioChanged)
    Q_PROPERTY(QString defaultExportQuality READ defaultExportQuality WRITE setDefaultExportQuality NOTIFY defaultExportQualityChanged)
    Q_PROPERTY(QString defaultExportFormat READ defaultExportFormat WRITE setDefaultExportFormat NOTIFY defaultExportFormatChanged)

public:
    explicit Settings(QObject *parent = nullptr);

    double defaultPadding() const;
    void setDefaultPadding(double value);

    QString defaultAspectRatio() const;
    void setDefaultAspectRatio(const QString &value);

    QString defaultExportQuality() const;
    void setDefaultExportQuality(const QString &value);

    QString defaultExportFormat() const;
    void setDefaultExportFormat(const QString &value);

    // Shortcuts persistence
    Q_INVOKABLE QVariantMap loadShortcuts();
    Q_INVOKABLE void saveShortcuts(const QVariantMap &shortcuts);

signals:
    void defaultPaddingChanged();
    void defaultAspectRatioChanged();
    void defaultExportQualityChanged();
    void defaultExportFormatChanged();

private:
    QSettings m_settings;
};

} // namespace screencopy
