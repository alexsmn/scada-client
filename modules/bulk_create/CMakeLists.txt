client_module(client_bulk_create CONFIG qt)

# Export the modules/ root so this module and its consumers can include
# "bulk_create/qt/bulk_create_preview_panel.h".
client_module_include_directories(client_bulk_create PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

client_module_link_libraries(client_bulk_create
  PUBLIC
    Qt6::Widgets
  PRIVATE
    aui
)

# The unit tests drive the pure pattern engine and the preview widget directly,
# so the test binary needs aui + a QApplication (aui_test_qt).
if(TARGET client_bulk_create_qt_unittests)
  target_link_libraries(client_bulk_create_qt_unittests PRIVATE
    aui_qt
    aui_test_qt
  )
endif()
