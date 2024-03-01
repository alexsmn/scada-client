add_library(client_common INTERFACE)

target_link_libraries(client_common INTERFACE
  base_i18n
  node_service_v1
  node_service_v2
  client_base
  client_clipboard
  client_core
  scada_common_events
  scada_common_vidicon_services
  scada_core_opcua
  scada_core_remote
  timed_data
  Net::net
)

add_subdirectory(qt)
add_subdirectory(wt)