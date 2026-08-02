client_component_module(client_component_about)

if(TARGET client_component_about_qt_unittests)
  # The show-dialog regression test needs a QApplication (aui_test_qt);
  # AboutDialog itself pulls LoadPixmap from client_ui_common.
  target_link_libraries(client_component_about_qt_unittests PRIVATE
    aui_test_qt
    client_ui_common_qt
  )
endif()
