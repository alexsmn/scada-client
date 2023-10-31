client_module(client_configuration_tree_lib)

client_module_link_libraries(client_configuration_tree_lib PUBLIC
  aui
  node_service
)

target_link_libraries(client_configuration_tree_lib_qt PRIVATE
  # TODO: Remove.
  client_qt_lib
)

target_link_libraries(client_configuration_tree_lib_wt PRIVATE
  # TODO: Remove.
  client_wt_lib
)
