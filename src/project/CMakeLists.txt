add_library(screencopy_project STATIC
    ProjectTypes.h
    ProjectData.h
    ProjectData.cpp
    ProjectManager.h
    ProjectManager.cpp
)

target_link_libraries(screencopy_project PUBLIC Qt6::Core)
target_include_directories(screencopy_project PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
