add_library(client_common INTERFACE)

target_link_libraries(client_common INTERFACE
  node_service_v1
  node_service_v2
  client_base
  client_clipboard
  client_core
  scada_common_events
  scada_common_vidicon_services
  scada_core_remote
  timed_data
  Transport::transport
)

# `client_application.cpp` and `startup_exception.cpp` are compiled once
# into `client_qt_app_shared` / `client_wt_lib` and linked into both the
# apps and the test binaries, so only the test sources themselves need to
# be added here.
list(APPEND UNITTESTS
  "${CMAKE_CURRENT_SOURCE_DIR}/startup_exception_unittest.cpp"
)

list(APPEND UNITTESTS_QT
  "${CMAKE_CURRENT_SOURCE_DIR}/client_application_unittest.cpp"
)

add_subdirectory(qt)
add_subdirectory(wt)
