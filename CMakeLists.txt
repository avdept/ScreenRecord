cmake_minimum_required(VERSION 3.22)
project(screencopy VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Enable Swift for macOS — the native ScreenCaptureKit/AVFoundation code lives
# in Swift modules that are linked into the C++ libraries via a C ABI bridge.
if(APPLE)
    # Floor: ScreenCaptureKit requires 12.3. SCRecordingOutput needs 15.0 —
    # guarded at runtime. We pick 14.0 as a practical floor (matches Qt 6.5+
    # deployment) while still supporting users on macOS 14 via fallbacks.
    if(NOT CMAKE_OSX_DEPLOYMENT_TARGET)
        set(CMAKE_OSX_DEPLOYMENT_TARGET "14.0")
    endif()
    enable_language(Swift)
endif()

# Find Qt6 modules
find_package(Qt6 6.5 REQUIRED COMPONENTS
    Core
    Gui
    Quick
    QuickControls2
    Widgets
    Multimedia
    Test
)

# Optional Qt6 modules
find_package(Qt6 COMPONENTS DBus WaylandClient QUIET)

qt_policy(SET QTP0001 NEW)

# Find FFmpeg via pkg-config
find_package(PkgConfig REQUIRED)
pkg_check_modules(AVCODEC REQUIRED IMPORTED_TARGET libavcodec)
pkg_check_modules(AVFORMAT REQUIRED IMPORTED_TARGET libavformat)
pkg_check_modules(AVUTIL REQUIRED IMPORTED_TARGET libavutil)
pkg_check_modules(SWSCALE REQUIRED IMPORTED_TARGET libswscale)
pkg_check_modules(SWRESAMPLE REQUIRED IMPORTED_TARGET libswresample)
pkg_check_modules(AVFILTER REQUIRED IMPORTED_TARGET libavfilter)

# Optional: PipeWire for Linux screen capture
if(UNIX AND NOT APPLE)
    pkg_check_modules(PIPEWIRE IMPORTED_TARGET libpipewire-0.3)
    if(PIPEWIRE_FOUND)
        add_compile_definitions(HAVE_PIPEWIRE)
    endif()
endif()

# Platform defines
if(Qt6DBus_FOUND)
    add_compile_definitions(HAVE_DBUS)
endif()
if(Qt6WaylandClient_FOUND)
    add_compile_definitions(HAVE_WAYLAND)
endif()

# Sub-libraries
add_subdirectory(src/project)
add_subdirectory(src/platform)
add_subdirectory(src/capture)
add_subdirectory(src/renderer)
add_subdirectory(src/editor)
add_subdirectory(src/exporter)
add_subdirectory(src/i18n)
add_subdirectory(src/app)

# Main executable
qt_add_executable(screencopy src/main.cpp)
set_target_properties(screencopy PROPERTIES
    OUTPUT_NAME screencopy-app
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_BUNDLE_NAME "ScreenCopy"
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.screencopy.app"
    MACOSX_BUNDLE_BUNDLE_VERSION "0.1.0"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "0.1.0"
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/resources/Info.plist.in"
    # Force C++ linker for the final executable — Qt/Clang linker flags aren't
    # understood by swiftc, and CMake otherwise defaults to swiftc when any
    # transitively-linked library contains Swift objects.
    LINKER_LANGUAGE CXX
)

# Sign the app bundle with a stable identity so macOS TCC retains permissions
# across rebuilds. Defaults to a self-signed "ScreenCopyDev" cert that each
# developer creates once locally (see README). Override with -DCODESIGN_IDENTITY=...
# Falls back to ad-hoc signing (-) with a warning if the cert isn't found.
if(APPLE)
    if(NOT DEFINED CODESIGN_IDENTITY)
        set(CODESIGN_IDENTITY "ScreenCopyDev")
    endif()

    execute_process(
        COMMAND security find-identity -v -p codesigning
        OUTPUT_VARIABLE _codesign_identities
        ERROR_QUIET
    )
    if(NOT "${_codesign_identities}" MATCHES "${CODESIGN_IDENTITY}")
        message(WARNING
            "Code signing identity '${CODESIGN_IDENTITY}' not found — falling back to ad-hoc. "
            "TCC permissions (Screen Recording, etc.) won't persist across rebuilds. "
            "See README for one-time setup.")
        set(CODESIGN_IDENTITY "-")
    endif()

    add_custom_command(TARGET screencopy POST_BUILD
        COMMAND codesign --force --deep --sign "${CODESIGN_IDENTITY}" "$<TARGET_BUNDLE_DIR:screencopy>"
        COMMENT "Codesigning screencopy with: ${CODESIGN_IDENTITY}"
        VERBATIM
    )
endif()

target_link_libraries(screencopy PRIVATE
    screencopy_app
    screencopy_capture
    screencopy_project
    screencopy_editor
    screencopy_renderer
    screencopy_exporter
    screencopy_platform
    screencopy_i18n
    Qt6::Quick
    Qt6::QuickControls2
)

# QML files — flattened into ScreenCopy module root
set(QML_FILES
    qml/Main.qml
    qml/HudWindow.qml
    qml/EditorWindow.qml
    qml/EditorTopBarButton.qml
    qml/Theme.qml
    qml/hud/HudOverlay.qml
    qml/hud/HudIconButton.qml
    qml/hud/HudGroupButton.qml
    qml/hud/DeviceSelector.qml
    qml/hud/AreaSelectionOverlay.qml
    qml/editor/VideoPlayback.qml
    qml/editor/SettingsPanel.qml
    qml/editor/PlaybackControls.qml
    qml/editor/timeline/TimelineEditor.qml
    qml/editor/timeline/TimelineToolButton.qml
    qml/components/AudioLevelMeter.qml
    qml/components/SettingsSection.qml
    qml/components/SettingsSlider.qml
)

# Flatten subdirectory QML files into module root
foreach(qml_file ${QML_FILES})
    get_filename_component(qml_name ${qml_file} NAME)
    set_source_files_properties(${qml_file} PROPERTIES QT_RESOURCE_ALIAS ${qml_name})
endforeach()

set_source_files_properties(qml/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

# QML module
qt_add_qml_module(screencopy
    URI ScreenCopy
    VERSION 1.0
    QML_FILES ${QML_FILES}
    RESOURCES
        resources/icons/screencopy.png
        resources/icons/rec-button.png
        resources/icons/volume-up.svg
        resources/icons/volume-off.svg
        resources/icons/mic.svg
        resources/icons/mic-off.svg
        resources/icons/videocam.svg
        resources/icons/videocam-off.svg
        resources/icons/monitor.svg
        resources/icons/video-file.svg
        resources/icons/restart.svg
        resources/icons/cancel.svg
        resources/icons/record.svg
        resources/icons/pause.svg
        resources/icons/play.svg
        resources/icons/stop.svg
        resources/icons/folder-open.svg
        resources/icons/minus.svg
        resources/icons/x-close.svg
        resources/icons/chevron-down.svg
        resources/icons/languages.svg
        resources/icons/drag-handle.svg
        resources/icons/play-black.svg
        resources/icons/pause-white.svg
        resources/icons/scissors.svg
        resources/icons/cursor-select.svg
        resources/icons/window.svg
        resources/wallpapers/wallpaper1.jpg
        resources/wallpapers/wallpaper2.jpg
        resources/wallpapers/wallpaper3.jpg
        resources/wallpapers/wallpaper4.jpg
        resources/wallpapers/wallpaper5.jpg
        resources/wallpapers/wallpaper6.jpg
        resources/wallpapers/wallpaper7.jpg
        resources/wallpapers/wallpaper8.jpg
        resources/wallpapers/wallpaper9.jpg
        resources/wallpapers/wallpaper10.jpg
        resources/wallpapers/wallpaper11.jpg
        resources/wallpapers/wallpaper12.jpg
        resources/wallpapers/wallpaper13.jpg
        resources/wallpapers/wallpaper14.jpg
        resources/wallpapers/wallpaper15.jpg
        resources/wallpapers/wallpaper16.jpg
        resources/wallpapers/wallpaper17.jpg
        resources/wallpapers/wallpaper18.jpg
)

install(TARGETS screencopy
    RUNTIME DESTINATION bin
    BUNDLE DESTINATION .
)

enable_testing()
add_subdirectory(tests)
