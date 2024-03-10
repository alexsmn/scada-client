client_module(client_configuration_devices)

client_module_link_libraries(client_configuration_devices PUBLIC
  client_controller
)

client_module_link_libraries(client_configuration_devices PRIVATE
  client_configuration_tree
  client_services
)
