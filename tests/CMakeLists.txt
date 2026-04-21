add_executable(tst_compositefilter tst_compositefilter.cpp)
target_link_libraries(tst_compositefilter PRIVATE
    Qt6::Test
    Qt6::Gui
    screencopy_exporter
    screencopy_project
)
add_test(NAME tst_compositefilter COMMAND tst_compositefilter)
