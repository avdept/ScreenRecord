set(PLATFORM_SOURCES
    PlatformIntegration.h
    PlatformIntegration.cpp
)

if(UNIX AND NOT APPLE)
    list(APPEND PLATFORM_SOURCES
        LinuxPlatform.h
        LinuxPlatform.cpp
        ScreenCastPortal.h
        ScreenCastPortal.cpp
    )
elseif(APPLE)
    enable_language(OBJCXX)
    list(APPEND PLATFORM_SOURCES
        MacPlatform.h
        MacPlatform.cpp
        MacScreenCapture.h
        MacScreenCapture.mm
    )
elseif(WIN32)
    list(APPEND PLATFORM_SOURCES WindowsPlatform.h WindowsPlatform.cpp)
endif()

add_library(screencopy_platform STATIC ${PLATFORM_SOURCES})

target_link_libraries(screencopy_platform PUBLIC Qt6::Core Qt6::Gui Qt6::Quick)
target_include_directories(screencopy_platform PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(APPLE)
    target_link_libraries(screencopy_platform PUBLIC "-weak_framework ScreenCaptureKit")
    set_source_files_properties(MacScreenCapture.mm PROPERTIES COMPILE_FLAGS "-fobjc-arc")
endif()

if(Qt6DBus_FOUND)
    target_link_libraries(screencopy_platform PUBLIC Qt6::DBus)
endif()
