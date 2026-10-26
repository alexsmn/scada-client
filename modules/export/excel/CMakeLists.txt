client_module(client_export_excel)

client_module_link_libraries(client_export_excel PRIVATE
  client_controller
  # ExportToExcel() lives beside the CSV writer it shares cell formatting with.
  client_export_csv
  scada_common
)

if(TARGET client_export_excel_qt_unittests)
  target_link_libraries(client_export_excel_qt_unittests PRIVATE aui_qt)
endif()
