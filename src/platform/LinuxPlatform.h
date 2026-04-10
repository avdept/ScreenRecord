#pragma once

#include "PlatformIntegration.h"

namespace screencopy {

class ScreenCastPortal;

class LinuxPlatform : public PlatformIntegration
{
    Q_OBJECT

public:
    explicit LinuxPlatform(QObject *parent = nullptr);

    bool isWayland() const override { return m_isWayland; }
    bool isHyprland() const override { return m_isHyprland; }
    QString platformName() const override { return "linux"; }

    // Source selection via XDG Desktop Portal
    void requestSourceSelection() override;

protected:
    void applyHudWindowRulesImpl(QWindow *window) override;

private:
    void detectEnvironment();
    void applyHyprlandRules();

    bool m_isWayland = false;
    bool m_isHyprland = false;
    ScreenCastPortal *m_portal = nullptr;
};

} // namespace screencopy
