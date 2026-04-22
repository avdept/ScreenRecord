#pragma once

#include "PlatformIntegration.h"

namespace screencopy {

class MacScreenCapture;

class MacPlatform : public PlatformIntegration
{
    Q_OBJECT

public:
    explicit MacPlatform(QObject *parent = nullptr);

    QString platformName() const override { return "macos"; }

    void requestSourceSelection() override;
    void selectFullScreen() override;
    void selectWindow() override;

    // Returns the SCContentFilter from the last source selection (caller takes ownership)
    void *takeLastFilter();

protected:
    void applyHudWindowRulesImpl(QWindow *window) override;

private:
    MacScreenCapture *m_screenCapture = nullptr;
};

} // namespace screencopy
