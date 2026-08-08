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

# Where the tier binaries come from. Two cases, and the difference is ADR 0011:
#
#   in a build that also builds the tiers (the superproject), the targets are
#   right here and $<TARGET_FILE:> is both exact and a real build dependency;
#
#   in a standalone client build they are not, because a tier is a separate
#   product with its own build tree and there is no shared bin/. So each one is
#   named explicitly -- no directory scanned, no layout assumed.
#
# What is NOT a case any more is silently doing nothing. This used to `return()`
# the moment a target was missing, so a partial build dropped the whole suite
# and reported success; that is the failure CLAUDE.md warns about under "Running
# Tests". Now it says which tier it lacks and what to pass.
set(_scada_e2e_missing_tiers)
set(_scada_e2e_from_targets TRUE)
foreach(_tier IN LISTS SCADA_E2E_TIER_TARGETS)
  if(NOT TARGET ${_tier})
    set(_scada_e2e_from_targets FALSE)
  endif()
endforeach()

if(NOT TARGET client_qt)
  message(STATUS
    "client_server_e2e_tests: skipped, client_qt is not in this build")
  return()
endif()

if(NOT _scada_e2e_from_targets)
  # SCADA_E2E_<TIER>_EXE, e.g. SCADA_E2E_IEC104_EXE.
  foreach(_tier IN LISTS SCADA_E2E_TIER_TARGETS)
    string(REPLACE "scada-" "" _short "${_tier}")
    string(TOUPPER "${_short}" _short)
    if(NOT SCADA_E2E_${_short}_EXE)
      list(APPEND _scada_e2e_missing_tiers "SCADA_E2E_${_short}_EXE")
    elseif(NOT EXISTS "${SCADA_E2E_${_short}_EXE}")
      message(FATAL_ERROR
        "SCADA_E2E_${_short}_EXE points at '${SCADA_E2E_${_short}_EXE}', which "
        "does not exist.")
    endif()
  endforeach()
  if(_scada_e2e_missing_tiers)
    set(_scada_e2e_hint "")
    foreach(_var IN LISTS _scada_e2e_missing_tiers)
      string(APPEND _scada_e2e_hint "\n    -D${_var}=<path>")
    endforeach()
    message(STATUS
      "client_server_e2e_tests: skipped, this build has no tier binaries.\n"
      "  The suite drives real tier processes, which are separate products "
      "(ADR 0011).\n"
      "  Build them, then re-configure naming each one:${_scada_e2e_hint}")
    return()
  endif()
endif()

# One place decides where each tier binary comes from, so the compile
# definitions below read the same either way.
function(_scada_e2e_tier_exe tier out_var)
  if(_scada_e2e_from_targets)
    set(${out_var} "$<TARGET_FILE:${tier}>" PARENT_SCOPE)
    return()
  endif()
  string(REPLACE "scada-" "" _short "${tier}")
  string(TOUPPER "${_short}" _short)
  set(${out_var} "${SCADA_E2E_${_short}_EXE}" PARENT_SCOPE)
endfunction()

foreach(_tier IN LISTS SCADA_E2E_TIER_TARGETS)
  string(REPLACE "scada-" "" _short "${_tier}")
  string(TOUPPER "${_short}" _short)
  _scada_e2e_tier_exe("${_tier}" _scada_e2e_exe_${_short})
endforeach()

if(APPLE)
  find_program(SCADA_E2E_SQLITE3_PROGRAM sqlite3 REQUIRED)
  set(SCADA_E2E_SQLITE3_EXE "${SCADA_E2E_SQLITE3_PROGRAM}")
else()
  set(SCADA_E2E_SQLITE3_EXE
      "${SCADA_E2E_BUNDLED_SQLITE3}")
endif()

# The harness carries common/test/e2e/iec61850_test_server.cpp, which includes
# libiec61850 headers; those arrive through the `iec61850pp` product. Resolved
# HERE rather than in common, next to the target that links it: common declares
# the source, but a find_package there would run for every consumer of common
# and drag libiec61850 into all nine tier builds, which link no harness at all.
#
# Until ADR 0011 this was only ever satisfied by accident — the superproject
# root put iec61850pp on the module path, and `target_link_libraries(...
# iec61850pp)` with no such target is not an error in CMake, it is a raw library
# name. So a standalone client configured cleanly and failed at the first
# #include, which is the same trap designer/CMakeLists.txt documents for
# scada_vds_runtime_api.
scada_find_products(iec61850pp)
find_package(iec61850pp REQUIRED)

add_executable(client_server_e2e_tests
  "client_server_e2e_test.cpp"
  "client_server_e2e_test_support.cpp"
)

target_precompile_headers(client_server_e2e_tests REUSE_FROM scada_base)

target_link_libraries(client_server_e2e_tests PRIVATE
  base_unittest
  client_base
  iec61850pp
  # The shared cluster harness: its sources, its include root and the paths to
  # common's own fixtures all travel with the target.
  scada_e2e_harness
  # Direct OPC UA sessions to the cluster proxy (the FileSystem read/write
  # tests exercise the aggregation seam without driving the Qt client).
  scada_opcua_bridge
  transport
)

target_compile_definitions(client_server_e2e_tests PRIVATE
  # SingleTier topology: one device tier plays the whole server.
  SCADA_E2E_SERVER_EXE="${_scada_e2e_exe_IEC104}"
  # Cluster topology: the real tier split behind an aggregating proxy.
  SCADA_E2E_CONFIG_EXE="${_scada_e2e_exe_CONFIG}"
  SCADA_E2E_HISTORIAN_EXE="${_scada_e2e_exe_HISTORIAN}"
  SCADA_E2E_PROXY_EXE="${_scada_e2e_exe_PROXY}"
  SCADA_E2E_IEC104_EXE="${_scada_e2e_exe_IEC104}"
  SCADA_E2E_MODBUS_EXE="${_scada_e2e_exe_MODBUS}"
  SCADA_E2E_IEC61850_EXE="${_scada_e2e_exe_IEC61850}"
  SCADA_E2E_FILESYSTEM_EXE="${_scada_e2e_exe_FILESYSTEM}"
  SCADA_E2E_CLIENT_EXE="$<TARGET_FILE:client_qt>"
  SCADA_E2E_SQLITE3_EXE="${SCADA_E2E_SQLITE3_EXE}"
)

# Only the in-build tiers are build dependencies. Binaries named by path were
# built by somebody else, in another tree, before this configure ran.
if(_scada_e2e_from_targets)
  add_dependencies(client_server_e2e_tests ${SCADA_E2E_TIER_TARGETS})
endif()
add_dependencies(client_server_e2e_tests
  client_qt
  client_qt_copy_translations
)

include(GoogleTest)
gtest_discover_tests(client_server_e2e_tests DISCOVERY_MODE PRE_TEST)
