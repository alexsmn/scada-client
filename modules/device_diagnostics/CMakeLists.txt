client_module(client_device_diagnostics CONFIG qt)

# Export the modules/ root so this module and its consumers can include
# "device_diagnostics/qt/device_diagnostics_panel.h".
client_module_include_directories(client_device_diagnostics PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

client_module_link_libraries(client_device_diagnostics
  PUBLIC
    Qt6::Widgets
    # NodeRef appears in the public header (the device + reading snapshots).
    node_service
  PRIVATE
    aui
    # Device diagnostic variable ids.
    scada_model
    # Live counter/link specs.
    timed_data
)

# The unit test drives the panel widget and the pure band mapping directly, so
# the test binary needs aui + a QApplication (aui_test_qt).
if(TARGET client_device_diagnostics_qt_unittests)
  target_link_libraries(client_device_diagnostics_qt_unittests PRIVATE
    aui_qt
    aui_test_qt
  )
endif()
