client_module(aui_test)

if(TARGET aui_test_qt)
  target_link_libraries(aui_test_qt PUBLIC aui_qt)
endif()
