client_module(aui_test)

find_package(wt REQUIRED)
target_link_libraries(aui_test_wt PUBLIC Wt::Test)
