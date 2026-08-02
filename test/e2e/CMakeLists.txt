if(NOT WIN32 AND NOT APPLE)
  return()
endif()

# The monolith `server` was retired (ADR 0001 Stage C); the E2E drives real tier
# binaries. The SingleTier topology runs one device tier (scada-iec104); the
# Cluster topology stands up the full tier split — a config tier, the three
# device edges, a historian, a file-store tier, and a client-facing aggregating
# proxy — so the multi-protocol hardware tree, historian-backed history, and
# proxied file-store flows get real coverage (ADR 0001 4b cleanup #2
# follow-up). The suite needs every tier binary.
set(SCADA_E2E_TIER_TARGETS
  scada-config scada-historian scada-proxy
  scada-iec104 scada-modbus scada-iec61850 scada-filesystem)
set(_scada_e2e_have_all_tiers TRUE)
foreach(_tier IN LISTS SCADA_E2E_TIER_TARGETS)
  if(NOT TARGET ${_tier})
    set(_scada_e2e_have_all_tiers FALSE)
  endif()
endforeach()
if(NOT _scada_e2e_have_all_tiers OR NOT TARGET client_qt)
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
  "${CMAKE_SOURCE_DIR}/common/test/e2e/e2e_cluster.cpp"
  "${CMAKE_SOURCE_DIR}/common/test/e2e/e2e_file_helpers.cpp"
  "${CMAKE_SOURCE_DIR}/common/test/e2e/e2e_process.cpp"
  "${CMAKE_SOURCE_DIR}/common/test/e2e/e2e_server_process.cpp"
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
  # Direct OPC UA sessions to the cluster proxy (the FileSystem read/write
  # tests exercise the aggregation seam without driving the Qt client).
  scada_opcua_bridge
  transport
)

target_compile_definitions(client_server_e2e_tests PRIVATE
  # SingleTier topology: one device tier plays the whole server.
  SCADA_E2E_SERVER_EXE="$<TARGET_FILE:scada-iec104>"
  # Cluster topology: the real tier split behind an aggregating proxy.
  SCADA_E2E_CONFIG_EXE="$<TARGET_FILE:scada-config>"
  SCADA_E2E_HISTORIAN_EXE="$<TARGET_FILE:scada-historian>"
  SCADA_E2E_PROXY_EXE="$<TARGET_FILE:scada-proxy>"
  SCADA_E2E_IEC104_EXE="$<TARGET_FILE:scada-iec104>"
  SCADA_E2E_MODBUS_EXE="$<TARGET_FILE:scada-modbus>"
  SCADA_E2E_IEC61850_EXE="$<TARGET_FILE:scada-iec61850>"
  SCADA_E2E_FILESYSTEM_EXE="$<TARGET_FILE:scada-filesystem>"
  SCADA_E2E_CLIENT_EXE="$<TARGET_FILE:client_qt>"
  SCADA_E2E_SERVER_FIXTURE_DIR="${CMAKE_SOURCE_DIR}/common/test/e2e/fixtures/server-data"
  SCADA_E2E_SERVER_SETTINGS_TEMPLATE="${CMAKE_SOURCE_DIR}/common/test/e2e/fixtures/server-settings.json"
  SCADA_E2E_CONFIGURATION_BASE_SQL="${CMAKE_SOURCE_DIR}/common/test/e2e/fixtures/configuration-base.sql"
  SCADA_E2E_CONFIGURATION_FIXTURE_SQL="${CMAKE_SOURCE_DIR}/common/test/e2e/fixtures/configuration-fixture.sql"
  SCADA_E2E_SQLITE3_EXE="${SCADA_E2E_SQLITE3_EXE}"
)

add_dependencies(client_server_e2e_tests
  ${SCADA_E2E_TIER_TARGETS}
  client_qt
  client_qt_copy_translations
)

include(GoogleTest)
gtest_discover_tests(client_server_e2e_tests DISCOVERY_MODE PRE_TEST)
