# Automatically link Qt executables to qtmain target on Windows
cmake_policy(SET CMP0020 NEW)

file(GLOB MAIN_QT CONFIGURE_DEPENDS
  "../../*.rc"
  "../../res/client.qrc"
  "../client_application.cpp"
  "../client_application.h"
  "../app_init.cpp"
  "../app_init.h"
  "../startup_exception.cpp"
  "../startup_exception.h"
  "main.cpp"
)
list(REMOVE_ITEM SOURCES_QT ${MAIN_QT})

# Core lib

set(COMPONENTS ${SOURCES} ${SOURCES_QT})
list(FILTER COMPONENTS INCLUDE REGEX [[_component\..*]])
list(REMOVE_ITEM SOURCES ${COMPONENTS})
list(REMOVE_ITEM SOURCES_QT ${COMPONENTS})

add_library(client_qt_core
  ${SOURCES}
  ${SOURCES_QT}
  "../client_application_modules.cpp"
  "../client_application_modules.h"
  "../opcua_services_module.cpp"
  "../opcua_services_module.h"
)

target_sources(client_qt_core INTERFACE ${COMPONENTS})

set_target_properties(client_qt_core PROPERTIES
  AUTOMOC ON
  AUTOUIC ON
  AUTORCC ON
)

target_link_libraries(client_qt_core PUBLIC
  aui_qt
  client_common
  client_configuration_qt
  client_controller_qt
  client_events_qt
  client_export_configuration_qt
  client_graph_qt
  client_profile_qt
  client_services_qt
)

client_qt_translations(client_qt_core
  "client_ru.ts"
)

# Full lib

add_library(client_qt_lib INTERFACE)

target_link_libraries(client_qt_lib INTERFACE
  client_qt_core
  client_export_csv_qt
  client_favorites_qt
  client_main_window_qt
  client_modus_qt
  client_portfolio_qt
  client_print_service_qt
  client_properties_qt
  scada_core_opcua
  client_vidicon_qt
)

# Exe

add_executable(client_qt WIN32 ${MAIN_QT})
target_link_libraries(client_qt PUBLIC client_qt_lib)
set_property(TARGET client_qt PROPERTY OUTPUT_NAME "client")
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

find_package(Qt5LinguistTools REQUIRED)
qt5_add_translation(TRANSLATIONS_OUT "client_ru.ts")

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

add_dependencies(client_qt client_qt_copy_translations)

install(FILES ${TRANSLATIONS_OUT}
  DESTINATION "bin/translations"
  COMPONENT Client)

install(TARGETS client_qt
  RUNTIME_DEPENDENCY_SET client_deps
  RUNTIME DESTINATION "bin" COMPONENT Client)

# UT-s

add_executable(client_qt_unittests ${UNITTESTS} ${UNITTESTS_QT})
# Uses the custom runner from `test/qt/test_main_qt.cpp`.
target_link_libraries(client_qt_unittests PRIVATE client_qt_lib base_unittest)
include(GoogleTest)
gtest_discover_tests(client_qt_unittests DISCOVERY_MODE PRE_TEST)

# The client_screenshot_generator executable lives in
# `tools/screenshot_generator/` — see that directory's CMakeLists.
