#include "ShortcutsManager.h"
#include "Settings.h"

namespace screencopy {

const QMap<QString, QString> ShortcutsManager::s_defaults = {
    {"addZoom", "Z"},
    {"addTrim", "T"},
    {"addSpeed", "S"},
    {"addAnnotation", "A"},
    {"addKeyframe", "F"},
    {"deleteSelected", "Ctrl+D"},
    {"playPause", "Space"},
};

ShortcutsManager::ShortcutsManager(Settings *settings, QObject *parent)
    : QObject(parent)
    , m_settings(settings)
{
    loadDefaults();
    loadFromSettings();
}

QString ShortcutsManager::shortcutFor(const QString &action) const
{
    return m_shortcuts.value(action);
}

bool ShortcutsManager::setShortcut(const QString &action, const QString &keySequence)
{
    auto conflict = conflictingAction(keySequence);
    if (!conflict.isEmpty() && conflict != action)
        return false;

    m_shortcuts[action] = keySequence;
    saveToSettings();
    emit shortcutsChanged();
    return true;
}

QString ShortcutsManager::conflictingAction(const QString &keySequence) const
{
    for (auto it = m_shortcuts.cbegin(); it != m_shortcuts.cend(); ++it) {
        if (it.value() == keySequence)
            return it.key();
    }
    return {};
}

void ShortcutsManager::resetToDefaults()
{
    m_shortcuts = s_defaults;
    saveToSettings();
    emit shortcutsChanged();
}

QVariantMap ShortcutsManager::allShortcuts() const
{
    QVariantMap map;
    for (auto it = m_shortcuts.cbegin(); it != m_shortcuts.cend(); ++it)
        map[it.key()] = it.value();
    return map;
}

void ShortcutsManager::loadDefaults()
{
    m_shortcuts = s_defaults;
}

void ShortcutsManager::loadFromSettings()
{
    auto saved = m_settings->loadShortcuts();
    for (auto it = saved.cbegin(); it != saved.cend(); ++it) {
        if (m_shortcuts.contains(it.key()))
            m_shortcuts[it.key()] = it.value().toString();
    }
}

void ShortcutsManager::saveToSettings()
{
    QVariantMap map;
    for (auto it = m_shortcuts.cbegin(); it != m_shortcuts.cend(); ++it)
        map[it.key()] = it.value();
    m_settings->saveShortcuts(map);
}

} // namespace screencopy
