add_library(client_common INTERFACE)

target_link_libraries(client_common INTERFACE
  node_service_v3
  client_base
  client_clipboard
  client_core
  scada_common_events
  scada_core_remote
  timed_data
  Transport::transport
)
if(TARGET scada_common_vidicon_services)
  target_link_libraries(client_common INTERFACE scada_common_vidicon_services)
endif()

# `client_application.cpp` is compiled once into `client_qt_app_shared` and
# linked into both the app and the heavy integration test binaries below, so
# only test sources themselves need to be added here.
list(APPEND UNITTESTS_QT
  "${CMAKE_CURRENT_SOURCE_DIR}/client_application_unittest.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/qt/e2e_test_support_unittest.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/qt/startup_flow_unittest.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/../tools/screenshot_generator/screenshot_wait.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/../tools/screenshot_generator/screenshot_wait_unittest.cpp"
)

# Small, platform-neutral test executable for `startup_exception`. The code
# under test has no Qt dependency — extracting it here means the test links
# only `scada_base` + `base_unittest` instead of the full
# `client_qt_app_shared` graph used by `client_qt_unittests`.
add_executable(client_startup_exception_tests
  "${CMAKE_CURRENT_SOURCE_DIR}/login_canceled.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/startup_exception.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/startup_exception.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/startup_exception_unittest.cpp"
)
target_include_directories(client_startup_exception_tests PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/.."
)
target_link_libraries(client_startup_exception_tests PRIVATE
  scada_base
  base_unittest
)
if(MSVC)
  target_link_options(client_startup_exception_tests PRIVATE
    "$<$<CONFIG:RelWithDebInfo>:/DEBUG:NONE>"
    "$<$<CONFIG:RelWithDebInfo>:/INCREMENTAL:NO>"
    "$<$<CONFIG:RelWithDebInfo>:/PDB:NONE>"
  )
endif()
include(GoogleTest)
gtest_discover_tests(client_startup_exception_tests DISCOVERY_MODE PRE_TEST)

if(BUILD_CLIENT_QT)
  add_subdirectory(qt)
endif()
