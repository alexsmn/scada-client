client_module(client_export_excel)

client_module_link_libraries(client_export_excel PRIVATE
  client_controller
  scada_common
)
