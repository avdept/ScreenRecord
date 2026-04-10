add_library(screencopy_app STATIC
    Application.h
    Application.cpp
    WindowManager.h
    WindowManager.cpp
    Settings.h
    Settings.cpp
)

target_link_libraries(screencopy_app PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    screencopy_editor
    screencopy_capture
    screencopy_platform
    screencopy_i18n
)
target_include_directories(screencopy_app PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
