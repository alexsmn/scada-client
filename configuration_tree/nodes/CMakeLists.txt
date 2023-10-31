client_module(client_configuration_tree_nodes)

client_module_link_libraries(client_configuration_tree_nodes PUBLIC
  client_controller
)

client_module_link_libraries(client_configuration_tree_nodes PRIVATE
  client_configuration_tree_lib
)
