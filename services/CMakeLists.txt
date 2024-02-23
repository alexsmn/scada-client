client_module(client_services)

client_module_link_libraries(client_services PUBLIC
  aui
  scada_common
)

# UTs.

target_link_libraries(client_services_qt_unittests PRIVATE
  # TODO: Remove this dependency.
  client_qt_lib
)