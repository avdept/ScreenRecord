#pragma once

#include <QObject>
#include <QTranslator>

namespace screencopy {

class I18nManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString currentLocale READ currentLocale WRITE setCurrentLocale NOTIFY currentLocaleChanged)
    Q_PROPERTY(QStringList availableLocales READ availableLocales CONSTANT)

public:
    explicit I18nManager(QObject *parent = nullptr);

    QString currentLocale() const { return m_currentLocale; }
    void setCurrentLocale(const QString &locale);

    QStringList availableLocales() const { return {"en", "zh-CN", "es"}; }

signals:
    void currentLocaleChanged();

private:
    QString m_currentLocale = "en";
    QTranslator m_translator;
};

} // namespace screencopy
