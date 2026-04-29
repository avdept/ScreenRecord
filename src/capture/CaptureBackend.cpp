#include "CaptureBackend.h"

#if defined(Q_OS_LINUX)
#include "LinuxCaptureBackend.h"
#elif defined(Q_OS_MACOS)
#include "MacCaptureBackend.h"
#endif

namespace screencopy {

CaptureBackend *CaptureBackend::create(PlatformIntegration *platform, QObject *parent)
{
#if defined(Q_OS_LINUX)
    return new LinuxCaptureBackend(platform, parent);
#elif defined(Q_OS_MACOS)
    return new MacCaptureBackend(platform, parent);
#else
    Q_UNUSED(platform);
    Q_UNUSED(parent);
    return nullptr;
#endif
}

} // namespace screencopy
