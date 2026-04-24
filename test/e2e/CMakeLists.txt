if(NOT WIN32)
  return()
endif()

if(NOT TARGET server OR NOT TARGET client_qt)
  return()
endif()

add_executable(client_server_e2e_tests
  "client_server_e2e_test.cpp"
  "client_server_e2e_test_support.cpp"
)

target_precompile_headers(client_server_e2e_tests REUSE_FROM scada_base)

target_link_libraries(client_server_e2e_tests PRIVATE
  base_unittest
  client_base
)

target_compile_definitions(client_server_e2e_tests PRIVATE
  SCADA_E2E_SERVER_EXE="$<TARGET_FILE:server>"
  SCADA_E2E_CLIENT_EXE="$<TARGET_FILE:client_qt>"
  SCADA_E2E_SERVER_FIXTURE_DIR="${CMAKE_SOURCE_DIR}/server/data"
)

add_dependencies(client_server_e2e_tests
  server
  client_qt
  client_qt_copy_translations
)

include(GoogleTest)
gtest_discover_tests(client_server_e2e_tests DISCOVERY_MODE PRE_TEST)
