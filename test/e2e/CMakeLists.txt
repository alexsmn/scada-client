if(NOT WIN32 AND NOT APPLE)
  return()
endif()

if(NOT TARGET server OR NOT TARGET client_qt)
  return()
endif()

if(APPLE)
  find_program(SCADA_E2E_SQLITE3_PROGRAM sqlite3 REQUIRED)
  set(SCADA_E2E_SQLITE3_EXE "${SCADA_E2E_SQLITE3_PROGRAM}")
else()
  set(SCADA_E2E_SQLITE3_EXE
      "${CMAKE_SOURCE_DIR}/common/test/e2e/fixtures/sqlite3.exe")
endif()

add_executable(client_server_e2e_tests
  "client_server_e2e_test.cpp"
  "client_server_e2e_test_support.cpp"
  "${CMAKE_SOURCE_DIR}/common/test/e2e/e2e_file_helpers.cpp"
  "${CMAKE_SOURCE_DIR}/common/test/e2e/e2e_process.cpp"
  "${CMAKE_SOURCE_DIR}/common/test/e2e/iec61850_test_server.cpp"
)

target_precompile_headers(client_server_e2e_tests REUSE_FROM scada_base)

target_include_directories(client_server_e2e_tests PRIVATE
  "${CMAKE_SOURCE_DIR}/common"
)

target_link_libraries(client_server_e2e_tests PRIVATE
  base_unittest
  client_base
  iec61850pp
)

target_compile_definitions(client_server_e2e_tests PRIVATE
  SCADA_E2E_SERVER_EXE="$<TARGET_FILE:server>"
  SCADA_E2E_CLIENT_EXE="$<TARGET_FILE:client_qt>"
  SCADA_E2E_SERVER_FIXTURE_DIR="${CMAKE_SOURCE_DIR}/common/test/e2e/fixtures/server-data"
  SCADA_E2E_SERVER_SETTINGS_TEMPLATE="${CMAKE_SOURCE_DIR}/common/test/e2e/fixtures/server-settings.json"
  SCADA_E2E_CONFIGURATION_BASE_SQL="${CMAKE_SOURCE_DIR}/common/test/e2e/fixtures/configuration-base.sql"
  SCADA_E2E_CONFIGURATION_FIXTURE_SQL="${CMAKE_SOURCE_DIR}/common/test/e2e/fixtures/configuration-fixture.sql"
  SCADA_E2E_SQLITE3_EXE="${SCADA_E2E_SQLITE3_EXE}"
)

add_dependencies(client_server_e2e_tests
  server
  client_qt
  client_qt_copy_translations
)

include(GoogleTest)
gtest_discover_tests(client_server_e2e_tests DISCOVERY_MODE PRE_TEST)
