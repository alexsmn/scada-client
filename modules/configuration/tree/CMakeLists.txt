client_module(client_configuration_tree)

client_module_link_libraries(client_configuration_tree PUBLIC
  aui
  node_service
  client_ui_dragdrop
  # `configuration_tree_drop_handler.cpp` uses `NetExecutorAdapter`, which
  # transitively pulls in `transport/executor.h`.
  transport
)

# configuration_tree_view_unittest.cpp drives the real view, so it needs the
# shared ControllerContext fixture (client_controller_test_qt), the Tree widget
# the row order lives on (aui_qt), and the standard node states the fixture
# seeds its address space from (address_space).
if(TARGET client_configuration_tree_qt_unittests)
  target_link_libraries(client_configuration_tree_qt_unittests PRIVATE
    address_space
    aui_qt
    client_controller_test_qt
  )
endif()
