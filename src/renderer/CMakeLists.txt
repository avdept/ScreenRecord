add_library(screencopy_renderer STATIC
    VideoPlayer.h
    VideoPlayer.cpp
)

target_link_libraries(screencopy_renderer PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    screencopy_project
    screencopy_exporter
    PkgConfig::AVCODEC
    PkgConfig::AVFORMAT
    PkgConfig::AVUTIL
    PkgConfig::SWSCALE
)
target_include_directories(screencopy_renderer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
