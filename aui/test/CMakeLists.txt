client_module(aui_test)

target_link_libraries(aui_test_qt PUBLIC aui_qt)

find_package(wt REQUIRED)
target_link_libraries(aui_test_wt PUBLIC Wt::Test)
