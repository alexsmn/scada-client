add_subdirectory(tree)

add_subdirectory(devices)
add_subdirectory(nodes)
add_subdirectory(objects)

client_module(client_configuration)

client_module_link_libraries(client_configuration PUBLIC
  client_controller
)

client_module_link_libraries(client_configuration PRIVATE
  client_configuration_devices
  client_configuration_nodes
  client_configuration_objects
)

if(TARGET client_configuration_qt)
  target_link_libraries(client_configuration_qt PRIVATE client_events_qt)
endif()
