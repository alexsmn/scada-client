client_module(client_configuration_tree_hardware)

client_module_link_libraries(client_configuration_tree_hardware PUBLIC
  client_controller
)

client_module_link_libraries(client_configuration_tree_hardware PRIVATE
  client_configuration_tree_lib
  client_services
)
