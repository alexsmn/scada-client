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
    # NOT a return(). External-target mode (SCADA_E2E_EXTERNAL_HOST) launches no
    # server at all, so building the suite to test a deployment used to cost
    # seven tier product builds whose output was never executed -- ~9 minutes of
    # compiling for binaries nobody ran. The target is produced either way; the
    # parameters that DO need a tier process skip at runtime instead, carrying
    # this same text. A tier path that is set but wrong is still a hard error
    # above: that is a mistake, whereas absence is now a supported mode.
    message(STATUS
      "client_server_e2e_tests: built without tier binaries. The hermetic "
      "parameters will skip at runtime;\n"
      "  external-target runs (SCADA_E2E_EXTERNAL_HOST) are unaffected.\n"
      "  The suite drives real tier processes, which are separate products "
      "(ADR 0011).\n"
      "  To run them, build the tiers and re-configure naming each "
      "one:${_scada_e2e_hint}")
  endif()
endif()

# What the runtime skip needs to name. Empty in a build that has its tiers --
# the hermetic parameters then run as they always did.
string(REPLACE ";" " " SCADA_E2E_MISSING_TIER_VARS "${_scada_e2e_missing_tiers}")

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

# ── The IEC 61850 device ────────────────────────────────────────────────────
#
# The suite needs a simulated IED for the `scada-iec61850` tier to talk to. It
# used to COMPILE one: `common/test/e2e/iec61850_test_server.cpp` travelled with
# the harness (an INTERFACE library, so its sources compile in the consumer),
# which put libiec61850 -- and with it the unpublished `iec61850pp` product and
# a git submodule -- into the Qt client's source closure for the sake of a fake
# device. That is what kept the public client from configuring on Windows and
# macOS, since `iec61850pp` is not beside it there.
#
# Since backlog 801 it is a BINARY, named by path exactly like the seven tier
# binaries above. A path that is set but wrong is a hard error -- that is a
# mistake -- while absence is a supported mode: the parameters that need a
# device skip at runtime, carrying the text below.
set(SCADA_E2E_IEC61850_DEVICE_EXE "" CACHE FILEPATH
    "The scada_e2e_iec61850_device binary, built by the superproject")
if(SCADA_E2E_IEC61850_DEVICE_EXE
   AND NOT EXISTS "${SCADA_E2E_IEC61850_DEVICE_EXE}")
  message(FATAL_ERROR
    "SCADA_E2E_IEC61850_DEVICE_EXE points at "
    "'${SCADA_E2E_IEC61850_DEVICE_EXE}', which does not exist.")
endif()
if(NOT SCADA_E2E_IEC61850_DEVICE_EXE)
  message(STATUS
    "client_server_e2e_tests: built without an IEC 61850 device. The "
    "parameters that need one will skip at runtime;\n"
    "  to have one, build the superproject's scada_e2e_iec61850_device target "
    "and re-configure with\n"
    "    -DSCADA_E2E_IEC61850_DEVICE_EXE=<path>")
endif()

add_executable(client_server_e2e_tests
  "client_server_e2e_test.cpp"
  "client_server_e2e_test_support.cpp"
)

target_precompile_headers(client_server_e2e_tests REUSE_FROM scada_base)

target_link_libraries(client_server_e2e_tests PRIVATE
  base_unittest
  client_base
  # The shared cluster harness: its sources, its include root and the paths to
  # common's own fixtures all travel with the target.
  scada_e2e_harness
  # Direct OPC UA sessions to the cluster proxy (the FileSystem read/write
  # tests exercise the aggregation seam without driving the Qt client).
  scada_opcua_bridge
  # The NATIVE (gRPC) client session -- CreateRemoteServices, the backend the
  # running client registers as "Scada". The transmission-add test drives both
  # transports in one body to check they still meet the same node-management
  # service, which is a property of the framework's wiring and not something a
  # single-protocol test can see.
  scada_core_remote
  transport
  # The Qt client's configuration transfer client (ADR 0014), driven over both
  # transports so the export/import path is checked through a real tier
  # rather than a fake transfer object.
  client_export_configuration_qt
)

target_compile_definitions(client_server_e2e_tests PRIVATE
  # The simulated IED, empty when this build was given none.
  SCADA_E2E_IEC61850_DEVICE_EXE="${SCADA_E2E_IEC61850_DEVICE_EXE}"
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
  # Empty unless this build has no tier binaries; the hermetic parameters read
  # it to skip with the same explanation the configure printed.
  SCADA_E2E_MISSING_TIER_VARS="${SCADA_E2E_MISSING_TIER_VARS}"
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
