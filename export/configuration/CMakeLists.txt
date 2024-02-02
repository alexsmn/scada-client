client_module(client_export_configuration)

client_module_link_libraries(client_export_configuration PUBLIC
  scada_common
)

client_module_link_libraries(client_export_configuration PRIVATE
  client_controller
)
