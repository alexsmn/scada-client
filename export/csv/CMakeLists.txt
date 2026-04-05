client_module(client_export_csv)

client_module_link_libraries(client_export_csv PUBLIC
  scada_common
)

client_module_link_libraries(client_export_csv PRIVATE
  client_controller
)
