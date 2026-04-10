add_library(screencopy_i18n STATIC
    I18nManager.h
    I18nManager.cpp
)

target_link_libraries(screencopy_i18n PUBLIC Qt6::Core Qt6::Gui)
target_include_directories(screencopy_i18n PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
