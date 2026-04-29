add_executable(tst_compositefilter tst_compositefilter.cpp)
target_link_libraries(tst_compositefilter PRIVATE
    Qt6::Test
    Qt6::Gui
    screencopy_exporter
    screencopy_project
)
add_test(NAME tst_compositefilter COMMAND tst_compositefilter)

if(APPLE)
    # macOS-only smoke tests: covers MacPlatform / MacCaptureBackend
    # construction and the Swift ↔ C ABI bridge wiring. Does NOT exercise
    # real screen capture — CI runners lack Screen Recording TCC.
    add_executable(tst_maccapture tst_maccapture.cpp)
    target_link_libraries(tst_maccapture PRIVATE
        Qt6::Test
        Qt6::Gui
        screencopy_capture
        screencopy_platform
    )
    add_test(NAME tst_maccapture COMMAND tst_maccapture)
endif()
