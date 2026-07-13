client_module(client_inspector CONFIG qt)

# Export the modules/ root so consumers can include "inspector/qt/...".
client_module_include_directories(client_inspector PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

client_module_link_libraries(client_inspector
  PUBLIC
    Qt6::Widgets
    # SelectionModel appears in the public header.
    client_controller
  PRIVATE
    aui
    timed_data
)

# The unit test exercises the quality-band mapping and drives the panel widget
# directly, so the test binary needs aui + a QApplication (aui_test_qt).
if(TARGET client_inspector_qt_unittests)
  target_link_libraries(client_inspector_qt_unittests PRIVATE
    aui_qt
    aui_test_qt
  )
endif()
