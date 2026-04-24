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
    list(APPEND PLATFORM_SOURCES
        MacPlatform.h
        MacPlatform.cpp
        MacScreenCapture.h
        MacScreenCapture.cpp
        MacScreenCaptureBridge.h
    )
elseif(WIN32)
    list(APPEND PLATFORM_SOURCES WindowsPlatform.h WindowsPlatform.cpp)
endif()

add_library(screencopy_platform STATIC ${PLATFORM_SOURCES})

target_link_libraries(screencopy_platform PUBLIC Qt6::Core Qt6::Gui Qt6::Quick)
target_include_directories(screencopy_platform PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(APPLE)
    # Swift module for ScreenCaptureKit source picker.
    # Linked PRIVATE into the C++ platform lib via the C ABI in MacScreenCaptureBridge.h.
    add_library(screencopy_platform_swift STATIC
        mac/Bridge.swift
        mac/ScreenCapturePicker.swift
        mac/HudWindowRules.swift
    )
    set_target_properties(screencopy_platform_swift PROPERTIES
        Swift_MODULE_NAME ScreenCapturePlatform
        AUTOMOC OFF
        AUTORCC OFF
    )
    # -import-objc-header is a Swift-only flag; wrap in $<COMPILE_LANGUAGE:Swift>.
    target_compile_options(screencopy_platform_swift PRIVATE
        "$<$<COMPILE_LANGUAGE:Swift>:SHELL:-import-objc-header ${CMAKE_CURRENT_SOURCE_DIR}/mac/PlatformBridging-Header.h>"
    )
    target_link_libraries(screencopy_platform_swift PRIVATE
        "-framework ScreenCaptureKit"
    )

    target_link_libraries(screencopy_platform PRIVATE screencopy_platform_swift)
endif()

if(Qt6DBus_FOUND)
    target_link_libraries(screencopy_platform PUBLIC Qt6::DBus)
endif()
