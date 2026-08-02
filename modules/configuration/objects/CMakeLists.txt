client_module(client_configuration_objects)

client_module_link_libraries(client_configuration_objects PUBLIC
  client_controller
)

client_module_link_libraries(client_configuration_objects PRIVATE
  address_space
  client_configuration_tree
  client_services
  timed_data
)
