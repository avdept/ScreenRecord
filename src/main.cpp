#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QIcon>

#include "app/Application.h"
#include "app/WindowManager.h"
#include "app/Settings.h"
#include "editor/PlaybackController.h"
#include "editor/GlobalSettings.h"
#include "editor/ShortcutsManager.h"
#include "editor/TrimModel.h"
#include "editor/EffectRegionModel.h"
#include "project/ProjectManager.h"
#include "capture/GpuScreenRecorder.h"
#include "capture/CursorTelemetry.h"
#include "capture/RecordingController.h"
#include "platform/PlatformIntegration.h"
#include "i18n/I18nManager.h"
#include "renderer/VideoPlayer.h"
#include "exporter/VideoExporter.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);
    app.setApplicationName("ScreenCopy");
    app.setOrganizationName("ScreenCopy");
    app.setApplicationVersion("0.1.0");
    app.setWindowIcon(QIcon(":/resources/icons/screencopy.png"));

    QQuickStyle::setStyle("Basic");

    // Core services
    auto *settings = new screencopy::Settings(&app);
    auto *i18n = new screencopy::I18nManager(&app);
    auto *platform = screencopy::PlatformIntegration::create(&app);
    auto *shortcuts = new screencopy::ShortcutsManager(settings, &app);
    auto *projectManager = new screencopy::ProjectManager(&app);
    auto *windowManager = new screencopy::WindowManager(&app);

    // Editor — split into playback + global rendering settings
    auto *playback = new screencopy::PlaybackController(&app);
    auto *globalSettings = new screencopy::GlobalSettings(projectManager, &app);
    auto *trimModel = new screencopy::TrimModel(&app);
    auto *effectRegions = new screencopy::EffectRegionModel(&app);

    // Export
    auto *videoExporter = new screencopy::VideoExporter(&app);
    videoExporter->setProjectManager(projectManager);
    videoExporter->setTrimModel(trimModel);
    videoExporter->setEffectRegions(effectRegions);

    // Capture
    auto *gpuRecorder = new screencopy::GpuScreenRecorder(platform, &app);
    auto *cursorTelemetry = new screencopy::CursorTelemetry(&app);
    auto *recorder = new screencopy::RecordingController(&app);
    recorder->setGpuRecorder(gpuRecorder);
    recorder->setPlatform(platform);
    recorder->setCursorTelemetry(cursorTelemetry);

    QQmlApplicationEngine engine;

    // Register QML types
    qmlRegisterType<screencopy::VideoPlayer>("ScreenCopy", 1, 0, "VideoPlayer");

    // Register singletons — organized by domain
    // App
    qmlRegisterSingletonInstance("ScreenCopy", 1, 0, "AppSettings", settings);
    qmlRegisterSingletonInstance("ScreenCopy", 1, 0, "I18n", i18n);
    qmlRegisterSingletonInstance("ScreenCopy", 1, 0, "Platform", platform);
    qmlRegisterSingletonInstance("ScreenCopy", 1, 0, "Shortcuts", shortcuts);
    qmlRegisterSingletonInstance("ScreenCopy", 1, 0, "WindowManager", windowManager);
    // Project
    qmlRegisterSingletonInstance("ScreenCopy", 1, 0, "ProjectManager", projectManager);
    // Editor
    qmlRegisterSingletonInstance("ScreenCopy", 1, 0, "Playback", playback);
    qmlRegisterSingletonInstance("ScreenCopy", 1, 0, "GlobalSettings", globalSettings);
    qmlRegisterSingletonInstance("ScreenCopy", 1, 0, "TrimModel", trimModel);
    qmlRegisterSingletonInstance("ScreenCopy", 1, 0, "EffectRegions", effectRegions);
    // Capture
    qmlRegisterSingletonInstance("ScreenCopy", 1, 0, "Recorder", recorder);
    // Export
    qmlRegisterSingletonInstance("ScreenCopy", 1, 0, "Exporter", videoExporter);

    engine.loadFromModule("ScreenCopy", "HudWindow");

    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
