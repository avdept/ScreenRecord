add_library(screencopy_capture STATIC
    CaptureBackend.h
    GpuScreenRecorder.h
    GpuScreenRecorder.cpp
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

if(APPLE)
    enable_language(OBJCXX)
    target_sources(screencopy_capture PRIVATE MacCaptureBackend.h MacCaptureBackend.mm)
    target_link_libraries(screencopy_capture PRIVATE
        "-weak_framework ScreenCaptureKit"
        "-framework AppKit"
        "-framework AVFoundation"
        "-framework CoreMedia"
    )
    set_source_files_properties(MacCaptureBackend.mm PROPERTIES COMPILE_FLAGS "-fobjc-arc")
endif()

if(PIPEWIRE_FOUND)
    target_sources(screencopy_capture PRIVATE PipeWireCapture.h PipeWireCapture.cpp)
    target_link_libraries(screencopy_capture PRIVATE PkgConfig::PIPEWIRE)
endif()
