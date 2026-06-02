# Automatically link Qt executables to qtmain target on Windows
cmake_policy(SET CMP0020 NEW)

file(GLOB MAIN_QT CONFIGURE_DEPENDS
  "../*.rc"
  "../../res/client.qrc"
  "../app_init.cpp"
  "../app_init.h"
  "main.cpp"
)
list(REMOVE_ITEM SOURCES_QT ${MAIN_QT})

# Shared library compiled once and linked into both `client_qt` and
# `client_qt_unittests`. Keeps the heavy `client_application.cpp` and the
# module-wiring support files off the hot path of a test-only rebuild —
# without it, every edit to `main.cpp` or an unrelated test source forces
# these files to be recompiled twice.
add_library(client_qt_app_shared STATIC
  "../client_application.cpp"
  "../client_application.h"
  "../client_application_modules.cpp"
  "../client_application_modules.h"
  "../../modules/opcua_services/opcua_services_module.cpp"
  "../../modules/opcua_services/opcua_services_module.h"
  "../startup_exception.cpp"
  "../startup_exception.h"
  "e2e_test_support.cpp"
  "e2e_test_support.h"
  "startup_flow.cpp"
  "startup_flow.h"
)
target_precompile_headers(client_qt_app_shared REUSE_FROM scada_base)
if(MSVC)
  target_compile_options(client_qt_app_shared PRIVATE
    /wd4702 # unreachable code in exception/test-support control flow
  )
endif()
target_link_libraries(client_qt_app_shared PUBLIC
  aui_qt
  client_common
  client_configuration_qt
  client_controller_qt
  client_events_qt
  client_export_configuration_qt
  client_graph_qt
  client_profile_qt
  client_services_qt
  ${CLIENT_QT_COMPONENT_TARGETS}
  client_ui_common_qt
  client_ui_dragdrop_qt
  client_ui_progress_qt
  client_export_csv_qt
  client_favorites_qt
  client_main_window_qt
  client_portfolio_qt
  client_print_service_qt
  client_properties_qt
  scada_core_opcua_client
)
if(TARGET client_modus_qt)
  target_link_libraries(client_qt_app_shared PUBLIC client_modus_qt)
  target_compile_definitions(client_qt_app_shared PUBLIC CLIENT_HAS_MODUS=1)
endif()
if(TARGET client_vidicon_qt)
  target_link_libraries(client_qt_app_shared PUBLIC client_vidicon_qt)
  target_compile_definitions(client_qt_app_shared PUBLIC CLIENT_HAS_VIDICON=1)
endif()

# Exe

add_executable(client_qt WIN32 ${MAIN_QT})
target_precompile_headers(client_qt REUSE_FROM scada_base)
target_link_libraries(client_qt PUBLIC client_qt_app_shared)
set_property(TARGET client_qt PROPERTY OUTPUT_NAME "client")
if(APPLE)
  set(CLIENT_MACOS_ICON "${CMAKE_CURRENT_SOURCE_DIR}/../../res/client.icns")
  target_sources(client_qt PRIVATE "${CLIENT_MACOS_ICON}")
  set_source_files_properties("${CLIENT_MACOS_ICON}" PROPERTIES
    MACOSX_PACKAGE_LOCATION "Resources")
  set_target_properties(client_qt PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_BUNDLE_NAME "Telecontrol SCADA Client"
    MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
    MACOSX_BUNDLE_GUI_IDENTIFIER "ru.telecontrol.scada.client"
    MACOSX_BUNDLE_ICON_FILE "client.icns"
    MACOSX_BUNDLE_INFO_STRING "Telecontrol SCADA Client"
    MACOSX_BUNDLE_LONG_VERSION_STRING "${PROJECT_VERSION}"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")
endif()
if(MSVC)
  target_link_options(client_qt PRIVATE
    "$<$<CONFIG:Debug>:/DEBUG:NONE>"
    "$<$<CONFIG:Debug>:/INCREMENTAL:NO>"
    "$<$<CONFIG:Debug>:/PDB:NONE>"
  )
endif()
if(COMMAND scada_add_windows_version_info)
  scada_add_windows_version_info(client_qt
    DESCRIPTION "Telecontrol SCADA Client"
    INTERNAL_NAME "SCADA Client"
    ORIGINAL_FILENAME "client.exe")
endif()

client_qt_translations(client_qt "app_ru.ts")

# Translations

add_custom_command(
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/client_ru.ts"
  DEPENDS ${CLIENT_QT_TRANSLATIONS}
  COMMAND "lconvert -i ${CLIENT_QT_TRANSLATIONS} -o ${CMAKE_CURRENT_BINARY_DIR}/client_ru.ts"
  COMMENT "Merge all translation files into ${CMAKE_CURRENT_BINARY_DIR}/client_ru.ts")

find_package(Qt6 COMPONENTS LinguistTools QUIET)
if(COMMAND qt_add_translation)
  qt_add_translation(TRANSLATIONS_OUT "client_ru.ts")

  add_custom_command(
    OUTPUT "${CMAKE_BINARY_DIR}/bin/$<CONFIG>/translations/client_ru.qm"
    DEPENDS ${TRANSLATIONS_OUT}
    COMMAND ${CMAKE_COMMAND} -E make_directory
      "${CMAKE_BINARY_DIR}/bin/$<CONFIG>/translations"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      ${TRANSLATIONS_OUT}
      "${CMAKE_BINARY_DIR}/bin/$<CONFIG>/translations/client_ru.qm"
    COMMENT "Copy translations to output directory")

  add_custom_target(client_qt_copy_translations
    DEPENDS "${CMAKE_BINARY_DIR}/bin/$<CONFIG>/translations/client_ru.qm")
else()
  add_custom_target(client_qt_copy_translations
    COMMAND ${CMAKE_COMMAND} -E echo
            "Qt LinguistTools were not found; skipping client translations."
    VERBATIM)
endif()

add_dependencies(client_qt client_qt_copy_translations)

if(TRANSLATIONS_OUT)
  install(FILES ${TRANSLATIONS_OUT}
    DESTINATION "bin/translations"
    COMPONENT Client)
endif()

install(TARGETS client_qt
  RUNTIME_DEPENDENCY_SET client_deps
  BUNDLE DESTINATION "." COMPONENT Client
  RUNTIME DESTINATION "bin" COMPONENT Client)

if(EXISTS "${CMAKE_SOURCE_DIR}/dependencies/bin")
  install(DIRECTORY "${CMAKE_SOURCE_DIR}/dependencies/bin/"
    DESTINATION "bin"
    COMPONENT Client)
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/examples/schemes")
  install(DIRECTORY "${CMAKE_SOURCE_DIR}/examples/schemes"
    DESTINATION "examples"
    COMPONENT Client)
endif()

# UT-s

add_executable(client_qt_unittests
  ${UNITTESTS}
  ${UNITTESTS_QT}
  "../../aui/test/qt/table_unittest.cpp"
  "../../aui/test/qt/tree_unittest.cpp"
)
target_precompile_headers(client_qt_unittests REUSE_FROM scada_base)
if(MSVC)
  target_link_options(client_qt_unittests PRIVATE
    "$<$<CONFIG:Debug>:/DEBUG:NONE>"
    "$<$<CONFIG:Debug>:/INCREMENTAL:NO>"
    "$<$<CONFIG:Debug>:/PDB:NONE>"
    "$<$<CONFIG:RelWithDebInfo>:/DEBUG:NONE>"
    "$<$<CONFIG:RelWithDebInfo>:/INCREMENTAL:NO>"
    "$<$<CONFIG:RelWithDebInfo>:/PDB:NONE>"
  )
endif()
# Uses the custom runner from `test/qt/test_main_qt.cpp`.
# `client_qt_app_shared` transitively supplies the Qt client link set.
target_link_libraries(client_qt_unittests PRIVATE
  client_qt_app_shared
  base_unittest
)
include(GoogleTest)
gtest_discover_tests(client_qt_unittests DISCOVERY_MODE PRE_TEST)

# The client_screenshot_generator executable lives in
# `tools/screenshot_generator/` — see that directory's CMakeLists.
