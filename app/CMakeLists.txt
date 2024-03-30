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

list(APPEND UNITTESTS
  "${CMAKE_CURRENT_SOURCE_DIR}/client_application.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/client_application.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/client_application_unittest.cpp"
)

add_subdirectory(qt)
add_subdirectory(wt)