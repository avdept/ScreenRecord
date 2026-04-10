add_library(screencopy_exporter STATIC
    ExportTypes.h
    FrameFilter.h
    FilterPipeline.h
    FilterPipeline.cpp
    VideoExporter.h
    VideoExporter.cpp
    FFmpegDecoder.h
    FFmpegDecoder.cpp
    FFmpegEncoder.h
    FFmpegEncoder.cpp
    FFmpegMuxer.h
    FFmpegMuxer.cpp
    AudioProcessor.h
    AudioProcessor.cpp
    GifExporter.h
    GifExporter.cpp
    filters/CompositeFilter.h
    filters/CompositeFilter.cpp
)

target_link_libraries(screencopy_exporter PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    screencopy_project
    PkgConfig::AVCODEC
    PkgConfig::AVFORMAT
    PkgConfig::AVUTIL
    PkgConfig::SWSCALE
    PkgConfig::SWRESAMPLE
    PkgConfig::AVFILTER
)
target_include_directories(screencopy_exporter PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/filters
    ${CMAKE_CURRENT_SOURCE_DIR}/../editor
)
