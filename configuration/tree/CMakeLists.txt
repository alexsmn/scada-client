client_module(client_configuration_tree)

client_module_link_libraries(client_configuration_tree PUBLIC
  aui
  node_service
)

target_link_libraries(client_configuration_tree_qt PRIVATE
  # TODO: Remove.
  client_qt_lib
)

target_link_libraries(client_configuration_tree_wt PRIVATE
  # TODO: Remove.
  client_wt_lib
)
