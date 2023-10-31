add_subdirectory(lib)
add_subdirectory(objects)
add_subdirectory(hardware)
add_subdirectory(nodes)

client_module(client_configuration_tree)

client_module_link_libraries(client_configuration_tree PUBLIC
  client_controller
)

client_module_link_libraries(client_configuration_tree PRIVATE
  client_configuration_tree_objects
  client_configuration_tree_hardware
  client_configuration_tree_nodes
)
