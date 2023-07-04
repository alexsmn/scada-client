macro(client_module_sources MODULE_NAME SOURCE_DIR)
  set(OPTIONS "RECURSE")
  set(ONE_VALUE_ARGS "")
  set(MULTI_VALUE_ARGS "")
  cmake_parse_arguments(CLIENT_MODULE "${OPTIONS}" "${ONE_VALUE_ARGS}" "${MULTI_VALUE_ARGS}" ${ARGN})

  if(CLIENT_MODULE_RECURSE)
    set(CLIENT_MODULE_GLOB "GLOB_RECURSE")
  else()
    set(CLIENT_MODULE_GLOB "GLOB")
  endif()

  message("${MODULE_NAME}: ${CLIENT_MODULE_GLOB} sources under ${SOURCE_DIR}")

  file(${CLIENT_MODULE_GLOB} ${MODULE_NAME}_SOURCES CONFIGURE_DEPENDS "${SOURCE_DIR}/*.cpp" "${SOURCE_DIR}/*.h")
  file(${CLIENT_MODULE_GLOB} ${MODULE_NAME}_UT_SOURCES CONFIGURE_DEPENDS "${SOURCE_DIR}/*_unittest.*" "${SOURCE_DIR}/*_mock.*")

  if (WIN32)
    file(GLOB ${MODULE_NAME}_SOURCES_WIN CONFIGURE_DEPENDS "${SOURCE_DIR}/win/*.cpp" "${SOURCE_DIR}/win/*.h")
    file(GLOB ${MODULE_NAME}_UT_SOURCES_WIN CONFIGURE_DEPENDS "${SOURCE_DIR}/win/*_unittest.*" "${SOURCE_DIR}/win/*_mock.*")
    list(APPEND ${MODULE_NAME}_SOURCES ${${MODULE_NAME}_SOURCES_WIN})
    list(APPEND ${MODULE_NAME}_UT_SOURCES ${${MODULE_NAME}_UT_SOURCES_WIN})
  endif()

  if (${MODULE_NAME}_UT_SOURCES)
    list(REMOVE_ITEM ${MODULE_NAME}_SOURCES ${${MODULE_NAME}_UT_SOURCES})
  endif()

  target_sources(${MODULE_NAME} PRIVATE ${${MODULE_NAME}_SOURCES})

  if (${MODULE_NAME}_UT_SOURCES)
    if (NOT TARGET ${MODULE_NAME}_UT_SOURCES)
      add_executable(${MODULE_NAME}_unittests ${${MODULE_NAME}_UT_SOURCES})
      target_link_libraries(${MODULE_NAME}_unittests PUBLIC ${MODULE_NAME} base_unittest)
      set_property(TARGET ${MODULE_NAME}_unittests PROPERTY FOLDER ${scada_folder})
      include(GoogleTest)
      # Temporary workaround of "no cmake script provided" error. Also,
      # `gtest_discover_tests` is not correctly handled by CTest. So you cannot test Linux builds.
      # In opposite, `gtest_add_tests` triggers generation on each UT modification.
      gtest_discover_tests(${MODULE_NAME}_unittests DISCOVERY_MODE PRE_TEST)
      # gtest_add_tests(TARGET ${MODULE_NAME}_unittests)
    endif()

    target_sources(${MODULE_NAME}_unittests PUBLIC ${${MODULE_NAME}_UT_SOURCES})
  endif()
endmacro()

macro(client_module MODULE_NAME)
  add_library(${MODULE_NAME})
  set_property(TARGET ${MODULE_NAME} PROPERTY FOLDER ${scada_folder})

  client_module_sources(${MODULE_NAME} ${CMAKE_CURRENT_SOURCE_DIR} ${ARGN})
endmacro()
