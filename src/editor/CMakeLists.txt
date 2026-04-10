add_library(screencopy_editor STATIC
    PlaybackController.h
    PlaybackController.cpp
    GlobalSettings.h
    GlobalSettings.cpp
    ShortcutsManager.h
    ShortcutsManager.cpp
    TrimModel.h
    TrimModel.cpp
    EffectRegionModel.h
    EffectRegionModel.cpp
)

target_link_libraries(screencopy_editor PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    screencopy_project
)
target_include_directories(screencopy_editor PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../app)
target_include_directories(screencopy_editor PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
