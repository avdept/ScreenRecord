#include "ScreenCastPortal.h"
#include <QDBusInterface>
#include <QDBusReply>
#include <QDBusUnixFileDescriptor>
#include <QCoreApplication>
#include <QDebug>

namespace screencopy {

static const QString PORTAL_SERVICE = "org.freedesktop.portal.Desktop";
static const QString PORTAL_PATH = "/org/freedesktop/portal/desktop";
static const QString PORTAL_SCREENCAST = "org.freedesktop.portal.ScreenCast";
static const QString PORTAL_REQUEST = "org.freedesktop.portal.Request";

ScreenCastPortal::ScreenCastPortal(QObject *parent)
    : QObject(parent)
    , m_bus(QDBusConnection::sessionBus())
{
    // Build the request object path prefix: /org/freedesktop/portal/desktop/request/SENDER/
    auto sender = m_bus.baseService().mid(1).replace('.', '_');
    m_requestPrefix = QString("/org/freedesktop/portal/desktop/request/%1/").arg(sender);
}

QString ScreenCastPortal::nextRequestToken()
{
    return QString("screencopy%1").arg(++m_tokenCounter);
}

void ScreenCastPortal::selectSource()
{
    if (hasSession()) {
        closeSession();
    }
    createSession();
}

void ScreenCastPortal::createSession()
{
    auto token = nextRequestToken();
    auto requestPath = m_requestPrefix + token;

    // Listen for the Response signal before making the call
    m_bus.connect(PORTAL_SERVICE, requestPath, PORTAL_REQUEST, "Response",
                  this, SLOT(onCreateSessionResponse(uint, QVariantMap)));

    QDBusInterface portal(PORTAL_SERVICE, PORTAL_PATH, PORTAL_SCREENCAST, m_bus);

    QVariantMap options;
    options["handle_token"] = token;
    options["session_handle_token"] = QString("screencopy_session_%1").arg(m_tokenCounter);

    portal.call("CreateSession", options);
}

void ScreenCastPortal::onCreateSessionResponse(uint response, const QVariantMap &results)
{
    if (response != 0) {
        emit error("Failed to create screen cast session");
        return;
    }

    m_sessionPath = QDBusObjectPath(results["session_handle"].toString());

    // Now call SelectSources to show the picker
    auto token = nextRequestToken();
    auto requestPath = m_requestPrefix + token;

    m_bus.connect(PORTAL_SERVICE, requestPath, PORTAL_REQUEST, "Response",
                  this, SLOT(onSelectSourcesResponse(uint, QVariantMap)));

    QDBusInterface portal(PORTAL_SERVICE, PORTAL_PATH, PORTAL_SCREENCAST, m_bus);

    QVariantMap options;
    options["handle_token"] = token;
    // Allow monitors + windows
    options["types"] = static_cast<uint>(Monitor | Window);
    // Allow multiple sources = false
    options["multiple"] = false;

    portal.call("SelectSources", m_sessionPath, options);
}

void ScreenCastPortal::onSelectSourcesResponse(uint response, const QVariantMap &results)
{
    Q_UNUSED(results);

    if (response != 0) {
        if (response == 1) {
            // User cancelled the picker
            closeSession();
            return;
        }
        emit error("Source selection failed");
        closeSession();
        return;
    }

    emit sourceSelected();

    // Automatically start the stream to get the PipeWire node ID
    startStream();
}

void ScreenCastPortal::startStream()
{
    if (!hasSession()) {
        emit error("No session active");
        return;
    }

    auto token = nextRequestToken();
    auto requestPath = m_requestPrefix + token;

    m_bus.connect(PORTAL_SERVICE, requestPath, PORTAL_REQUEST, "Response",
                  this, SLOT(onStartResponse(uint, QVariantMap)));

    QDBusInterface portal(PORTAL_SERVICE, PORTAL_PATH, PORTAL_SCREENCAST, m_bus);

    QVariantMap options;
    options["handle_token"] = token;

    // parent_window is empty string for no parent
    portal.call("Start", m_sessionPath, QString(), options);
}

void ScreenCastPortal::onStartResponse(uint response, const QVariantMap &results)
{
    if (response != 0) {
        emit error("Failed to start screen cast stream");
        closeSession();
        return;
    }

    // Parse streams from results
    // streams is an array of (node_id, properties) tuples
    auto streams = results["streams"];
    if (streams.canConvert<QDBusArgument>()) {
        const QDBusArgument &arg = streams.value<QDBusArgument>();
        arg.beginArray();
        while (!arg.atEnd()) {
            arg.beginStructure();
            uint nodeId;
            QVariantMap props;
            arg >> nodeId >> props;
            arg.endStructure();

            m_nodeId = nodeId;
            qDebug() << "ScreenCast: PipeWire node" << nodeId
                     << "source_type:" << props.value("source_type").toUInt();
            break; // use first stream
        }
        arg.endArray();
    }

    if (m_nodeId > 0) {
        emit streamReady(m_nodeId);
    } else {
        emit error("No streams returned from portal");
        closeSession();
    }
}

void ScreenCastPortal::closeSession()
{
    if (hasSession()) {
        QDBusInterface session(PORTAL_SERVICE, m_sessionPath.path(),
                               "org.freedesktop.portal.Session", m_bus);
        session.call("Close");
        m_sessionPath = QDBusObjectPath();
        m_nodeId = 0;
    }
}

} // namespace screencopy
