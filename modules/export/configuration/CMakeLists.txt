client_module(client_export_configuration)

client_module_link_libraries(client_export_configuration PUBLIC
  scada_common
  # The result document's reader (address_space/configuration_nodeset.h).
  address_space
  # The client's coroutine calls pull in `transport/executor.h`.
  transport
)

client_module_link_libraries(client_export_configuration PRIVATE
  client_controller
  # StatusText: the client's words for a status code.
  client_services
  # A result document's status codes are OPC UA wire codes; the client words
  # scada::StatusCode.
  scada_opcua_bridge
)

# UTs.

if(TARGET client_export_configuration_qt_unittests)
  target_link_libraries(client_export_configuration_qt_unittests PRIVATE
    aui_qt
  )
endif()
