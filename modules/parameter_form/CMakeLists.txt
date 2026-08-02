client_module(client_parameter_form CONFIG qt)

# Export the modules/ root so this module and its consumers can include
# "parameter_form/qt/device_parameter_form.h".
client_module_include_directories(client_parameter_form PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

client_module_link_libraries(client_parameter_form
  PUBLIC
    Qt6::Widgets
  PRIVATE
    # aui::PropertyModel + theme tokens + Translate.
    aui
)

# The unit tests drive the staging buffer and the form widget (against a fake
# PropertyModel) directly, so the test binary needs aui + a QApplication
# (aui_test_qt).
if(TARGET client_parameter_form_qt_unittests)
  target_link_libraries(client_parameter_form_qt_unittests PRIVATE
    aui_qt
    aui_test_qt
  )
endif()
