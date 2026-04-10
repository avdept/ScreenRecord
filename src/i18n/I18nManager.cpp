#include "I18nManager.h"
#include <QGuiApplication>
#include <QDir>

namespace screencopy {

I18nManager::I18nManager(QObject *parent)
    : QObject(parent)
{
}

void I18nManager::setCurrentLocale(const QString &locale)
{
    if (m_currentLocale == locale)
        return;

    QGuiApplication::instance()->removeTranslator(&m_translator);

    QString tsFile = QString(":/translations/openscreen_%1").arg(locale);
    if (m_translator.load(tsFile)) {
        QGuiApplication::instance()->installTranslator(&m_translator);
    }

    m_currentLocale = locale;
    emit currentLocaleChanged();
}

} // namespace screencopy
