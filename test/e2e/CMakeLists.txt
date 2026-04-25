if(NOT WIN32)
  return()
endif()

if(NOT TARGET server OR NOT TARGET client_qt)
  return()
endif()

add_executable(client_server_e2e_tests
  "client_server_e2e_test.cpp"
  "client_server_e2e_test_support.cpp"
  "e2e_file_helpers.cpp"
  "e2e_process.cpp"
  "iec61850_test_server.cpp"
)

target_precompile_headers(client_server_e2e_tests REUSE_FROM scada_base)

target_link_libraries(client_server_e2e_tests PRIVATE
  base_unittest
  client_base
  iec61850pp
)

target_compile_definitions(client_server_e2e_tests PRIVATE
  SCADA_E2E_SERVER_EXE="$<TARGET_FILE:server>"
  SCADA_E2E_CLIENT_EXE="$<TARGET_FILE:client_qt>"
  SCADA_E2E_SERVER_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fixtures/server-data"
  SCADA_E2E_SERVER_SETTINGS_TEMPLATE="${CMAKE_CURRENT_SOURCE_DIR}/fixtures/server-settings.json"
  SCADA_E2E_CONFIGURATION_BASE_SQL="${CMAKE_CURRENT_SOURCE_DIR}/fixtures/configuration-base.sql"
  SCADA_E2E_CONFIGURATION_FIXTURE_SQL="${CMAKE_CURRENT_SOURCE_DIR}/fixtures/configuration-fixture.sql"
  SCADA_E2E_SQLITE3_EXE="${CMAKE_CURRENT_SOURCE_DIR}/fixtures/sqlite3.exe"
)

add_dependencies(client_server_e2e_tests
  server
  client_qt
  client_qt_copy_translations
)

include(GoogleTest)
gtest_discover_tests(client_server_e2e_tests DISCOVERY_MODE PRE_TEST)
