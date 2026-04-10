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

if(PIPEWIRE_FOUND)
    target_sources(screencopy_capture PRIVATE PipeWireCapture.h PipeWireCapture.cpp)
    target_link_libraries(screencopy_capture PRIVATE PkgConfig::PIPEWIRE)
endif()
