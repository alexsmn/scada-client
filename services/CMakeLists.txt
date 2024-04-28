client_module(client_services)

client_module_link_libraries(client_services PUBLIC
  aui
  scada_common
)

# UTs.

target_link_libraries(client_services_qt_unittests PRIVATE
  address_space
  client_core
  client_events_qt
  client_profile_qt
  node_service
)

target_link_libraries(client_services_wt_unittests PRIVATE
  address_space
  client_core
  client_events_wt
  client_profile_wt
  node_service
)