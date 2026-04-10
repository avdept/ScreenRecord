#pragma once

#include <QObject>
#include <QDBusObjectPath>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>

namespace screencopy {

class ScreenCastPortal : public QObject
{
    Q_OBJECT

public:
    enum SourceType {
        Monitor = 1,
        Window  = 2,
        Virtual = 4
    };
    Q_DECLARE_FLAGS(SourceTypes, SourceType)

    explicit ScreenCastPortal(QObject *parent = nullptr);

    // Show the portal picker dialog. Emits sourceSelected on success.
    Q_INVOKABLE void selectSource();

    // Start the stream (after source selected). Emits streamReady with PipeWire node ID.
    Q_INVOKABLE void startStream();

    // Close the current session
    Q_INVOKABLE void closeSession();

    bool hasSession() const { return !m_sessionPath.path().isEmpty(); }
    uint pipeWireNodeId() const { return m_nodeId; }

signals:
    void sourceSelected();
    void streamReady(uint nodeId);
    void error(const QString &message);

private slots:
    void onCreateSessionResponse(uint response, const QVariantMap &results);
    void onSelectSourcesResponse(uint response, const QVariantMap &results);
    void onStartResponse(uint response, const QVariantMap &results);

private:
    void createSession();
    QString nextRequestToken();

    QDBusConnection m_bus;
    QDBusObjectPath m_sessionPath;
    QString m_requestPrefix;
    uint m_nodeId = 0;
    int m_tokenCounter = 0;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(ScreenCastPortal::SourceTypes)

} // namespace screencopy
