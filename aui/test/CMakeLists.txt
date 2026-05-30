client_module(aui_test)

if(TARGET aui_test_qt)
  target_link_libraries(aui_test_qt PUBLIC aui_qt)
endif()

if(TARGET aui_test_wt)
  find_package(wt REQUIRED)
  target_link_libraries(aui_test_wt PUBLIC Wt::Test)
  set_target_properties(aui_test_wt PROPERTIES DISABLE_PRECOMPILE_HEADERS ON)
endif()
