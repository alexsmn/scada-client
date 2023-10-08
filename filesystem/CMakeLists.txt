client_module(client_filesystem)

client_module_link_libraries(client_filesystem PUBLIC
  scada_common
)

client_module_link_libraries(client_filesystem PRIVATE
  client_controller
)
