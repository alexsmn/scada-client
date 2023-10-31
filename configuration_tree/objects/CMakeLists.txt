client_module(client_configuration_tree_objects)

client_module_link_libraries(client_configuration_tree_objects PUBLIC
  client_controller
)

client_module_link_libraries(client_configuration_tree_objects PRIVATE
  client_configuration_tree_lib
)
