add_library(screencopy_capture STATIC
    CaptureBackend.h
    CaptureBackend.cpp
    CursorTelemetry.h
    CursorTelemetry.cpp
    AudioCapture.h
    AudioCapture.cpp
    RecordingController.h
    RecordingController.cpp
)

target_link_libraries(screencopy_capture PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Multimedia
    screencopy_platform
    screencopy_project
)

target_include_directories(screencopy_capture PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(UNIX AND NOT APPLE)
    target_sources(screencopy_capture PRIVATE
        LinuxCaptureBackend.h
        LinuxCaptureBackend.cpp
    )
endif()

if(APPLE)
    target_sources(screencopy_capture PRIVATE
        MacCaptureBackend.h
        MacCaptureBackend.cpp
        MacCaptureBridge.h
    )

    # Swift recorder — linked PRIVATE via C ABI (MacCaptureBridge.h).
    add_library(screencopy_capture_swift STATIC
        mac/RecordingBridge.swift
        mac/ScreenRecorder.swift
    )
    set_target_properties(screencopy_capture_swift PROPERTIES
        Swift_MODULE_NAME ScreenCaptureRecorder
        AUTOMOC OFF
        AUTORCC OFF
    )
    target_compile_options(screencopy_capture_swift PRIVATE
        "$<$<COMPILE_LANGUAGE:Swift>:SHELL:-import-objc-header ${CMAKE_CURRENT_SOURCE_DIR}/mac/CaptureBridging-Header.h>"
    )
    target_link_libraries(screencopy_capture_swift PRIVATE
        "-framework ScreenCaptureKit"
        "-framework AVFoundation"
        "-framework AppKit"
        "-framework CoreMedia"
    )

    target_link_libraries(screencopy_capture PRIVATE
        screencopy_capture_swift
    )
endif()

if(PIPEWIRE_FOUND)
    target_sources(screencopy_capture PRIVATE PipeWireCapture.h PipeWireCapture.cpp)
    target_link_libraries(screencopy_capture PRIVATE PkgConfig::PIPEWIRE)
endif()
