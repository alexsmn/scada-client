client_module(client_export_csv)

client_module_link_libraries(client_export_csv PUBLIC
  scada_common
  # `csv_export_command.cpp` uses coroutine awaitables, which transitively pull
  # in `transport/executor.h`.
  transport
)

client_module_link_libraries(client_export_csv PRIVATE
  client_controller
)

if(TARGET client_export_csv_qt_unittests)
  target_link_libraries(client_export_csv_qt_unittests PRIVATE aui_qt)
endif()
if(TARGET client_export_csv_wt_unittests)
  target_link_libraries(client_export_csv_wt_unittests PRIVATE aui_wt)
endif()
