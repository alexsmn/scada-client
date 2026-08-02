client_module(client_export_configuration)

client_module_link_libraries(client_export_configuration PUBLIC
  scada_common
  # `excel_configuration_commands.cpp` uses coroutine awaitables, which
  # transitively pull in `transport/executor.h`.
  transport
)

client_module_link_libraries(client_export_configuration PRIVATE
  client_controller
)

# UTs.

if(TARGET client_export_configuration_qt_unittests)
  target_link_libraries(client_export_configuration_qt_unittests PRIVATE
    address_space
    aui_qt
  )
endif()
if(TARGET client_export_configuration_wt_unittests)
  target_link_libraries(client_export_configuration_wt_unittests PRIVATE
    address_space
    aui_wt
  )
endif()
