#include "LinuxPlatform.h"
#include "ScreenCastPortal.h"
#include <QProcess>
#include <QProcessEnvironment>
#include <QWindow>
#include <QScreen>
#include <QGuiApplication>

namespace screencopy {

static const QString HUD_WINDOW_TITLE = "screencopy-hud";

LinuxPlatform::LinuxPlatform(QObject *parent)
    : PlatformIntegration(parent)
{
    detectEnvironment();

    m_portal = new ScreenCastPortal(this);

    connect(m_portal, &ScreenCastPortal::streamReady, this, [this](uint nodeId) {
        auto sourceId = QString("pipewire:%1").arg(nodeId);
        auto sourceName = QString("Screen (PipeWire %1)").arg(nodeId);
        emit sourceSelected(sourceId, sourceName);
    });

    connect(m_portal, &ScreenCastPortal::error, this, [this](const QString &msg) {
        emit sourceSelectionError(msg);
    });
}

void LinuxPlatform::detectEnvironment()
{
    auto env = QProcessEnvironment::systemEnvironment();
    m_isWayland = env.value("XDG_SESSION_TYPE") == "wayland"
                  || !env.value("WAYLAND_DISPLAY").isEmpty();
    m_isHyprland = m_isWayland
                   && !env.value("HYPRLAND_INSTANCE_SIGNATURE").isEmpty();

}

void LinuxPlatform::applyHudWindowRulesImpl(QWindow *window)
{
    if (!window)
        return;

    if (m_isHyprland) {
        applyHyprlandRules();
    }
}

void LinuxPlatform::applyHyprlandRules()
{
    // Match the Electron app's Hyprland rule syntax exactly.
    // Uses the modern `keyword windowrule` with `match:title` syntax.
    auto appClass = QGuiApplication::applicationName();

    QStringList rules = {
        QString("float on, match:title ^(%1)$").arg(HUD_WINDOW_TITLE),
        QString("pin on, match:title ^(%1)$").arg(HUD_WINDOW_TITLE),
        QString("border_size 0, match:title ^(%1)$").arg(HUD_WINDOW_TITLE),
        QString("no_shadow on, match:title ^(%1)$").arg(HUD_WINDOW_TITLE),
        QString("no_anim on, match:title ^(%1)$").arg(HUD_WINDOW_TITLE),
        QString("no_blur on, match:title ^(%1)$").arg(HUD_WINDOW_TITLE),
        QString("move ((monitor_w-window_w)/2) (monitor_h-window_h-20), match:title ^(%1)$").arg(HUD_WINDOW_TITLE),
        QString("border_size 0, match:class ^(%1)$").arg(appClass),
        QString("no_shadow on, match:class ^(%1)$").arg(appClass),
    };

    QStringList batch;
    for (const auto &rule : rules)
        batch << QString("keyword windowrule %1").arg(rule);

    auto batchStr = batch.join(";");

    QProcess::execute("hyprctl", {"--batch", batchStr});
}

void LinuxPlatform::requestSourceSelection()
{
    m_portal->selectSource();
}

} // namespace screencopy
