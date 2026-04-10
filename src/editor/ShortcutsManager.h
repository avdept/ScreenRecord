#pragma once

#include <QObject>
#include <QKeySequence>
#include <QMap>

namespace screencopy {

class Settings;

class ShortcutsManager : public QObject
{
    Q_OBJECT

public:
    explicit ShortcutsManager(Settings *settings, QObject *parent = nullptr);

    Q_INVOKABLE QString shortcutFor(const QString &action) const;
    Q_INVOKABLE bool setShortcut(const QString &action, const QString &keySequence);
    Q_INVOKABLE QString conflictingAction(const QString &keySequence) const;
    Q_INVOKABLE void resetToDefaults();
    Q_INVOKABLE QVariantMap allShortcuts() const;

signals:
    void shortcutsChanged();

private:
    void loadDefaults();
    void loadFromSettings();
    void saveToSettings();

    Settings *m_settings;
    QMap<QString, QString> m_shortcuts;

    static const QMap<QString, QString> s_defaults;
};

} // namespace screencopy
