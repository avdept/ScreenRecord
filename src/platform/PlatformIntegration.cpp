#include "PlatformIntegration.h"
#include <QDesktopServices>
#include <QStandardPaths>
#include <QUrl>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QQuickWindow>

#ifdef Q_OS_LINUX
#include "LinuxPlatform.h"
#elif defined(Q_OS_MACOS)
#include "MacPlatform.h"
#endif

namespace screencopy {

PlatformIntegration::PlatformIntegration(QObject *parent)
    : QObject(parent)
{
}

PlatformIntegration *PlatformIntegration::create(QObject *parent)
{
#ifdef Q_OS_LINUX
    return new LinuxPlatform(parent);
#elif defined(Q_OS_MACOS)
    return new MacPlatform(parent);
#elif defined(Q_OS_WIN)
    // TODO: return new WindowsPlatform(parent);
    return nullptr;
#else
    return nullptr;
#endif
}

QString PlatformIntegration::recordingsDir() const
{
    auto dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/recordings";
    QDir().mkpath(dir);
    return dir;
}

void PlatformIntegration::applyHudWindowRules(QObject *window)
{
    // QML passes ApplicationWindow which is a QQuickWindow
    auto *qwindow = qobject_cast<QQuickWindow *>(window);
    if (qwindow) {
        applyHudWindowRulesImpl(qwindow);
    }
}

void PlatformIntegration::startRecording(const QString &outputPath) { Q_UNUSED(outputPath); }
void PlatformIntegration::stopRecording() {}
void PlatformIntegration::pauseRecording() {}
void PlatformIntegration::resumeRecording() {}
void PlatformIntegration::cancelRecording() {}

void PlatformIntegration::revealInFolder(const QString &filePath)
{
#ifdef Q_OS_LINUX
    QProcess::startDetached("xdg-open", {QFileInfo(filePath).absolutePath()});
#else
    QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(filePath).absolutePath()));
#endif
}

void PlatformIntegration::openExternalUrl(const QString &url)
{
    QDesktopServices::openUrl(QUrl(url));
}

} // namespace screencopy
