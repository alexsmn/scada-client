# Automatically link Qt executables to qtmain target on Windows
cmake_policy(SET CMP0020 NEW)

file(GLOB MAIN_QT CONFIGURE_DEPENDS
  "../../*.rc"
  "../../res/client.qrc"
  "../client_application.cpp"
  "../client_application.h"
  "../app_init.cpp"
  "../app_init.h"
  "main.cpp"
)
list(REMOVE_ITEM SOURCES_QT ${MAIN_QT})

# Lib

set(COMPONENTS ${SOURCES} ${SOURCES_QT})
list(FILTER COMPONENTS INCLUDE REGEX [[_component\..*]])
list(REMOVE_ITEM SOURCES ${COMPONENTS})
list(REMOVE_ITEM SOURCES_QT ${COMPONENTS})

add_library(client_qt_lib
  ${SOURCES}
  ${SOURCES_QT}
)

target_sources(client_qt_lib INTERFACE ${COMPONENTS})

set_target_properties(client_qt_lib PROPERTIES
  AUTOMOC ON
  AUTOUIC ON
  AUTORCC ON
)

target_link_libraries(client_qt_lib PUBLIC
  aui_qt
  client_common
  client_configuration_qt
  client_controller_qt
  client_events_qt
  client_export_configuration_qt
  client_export_csv_qt
  client_favorites_qt
  client_graph_qt
  client_main_window_qt
  client_modus_qt
  client_portfolio_qt
  client_print_preview_qt
  client_print_service_qt
  client_profile_qt
  client_properties_qt
  client_services_qt
  client_vidicon_qt
)

client_qt_translations(client_qt_lib
  "client_ru.ts"
)

# Exe

add_executable(client_qt WIN32 ${MAIN_QT})
target_link_libraries(client_qt PUBLIC client_qt_lib)
set_property(TARGET client_qt PROPERTY OUTPUT_NAME "client")

client_qt_translations(client_qt "app_ru.ts")

# Translations

add_custom_command(
  OUTPUT "client_ru.ts"
  DEPENDS ${CLIENT_QT_TRANSLATIONS}
  COMMAND "lconvert -i ${CLIENT_QT_TRANSLATIONS} -o client_ru.ts"
  COMMENT "Combine ${CLIENT_QT_TRANSLATIONS} into client_ru.ts")

find_package(Qt5LinguistTools REQUIRED)
qt5_add_translation(TRANSLATIONS_OUT "client_ru.ts")

file(COPY ${TRANSLATIONS_OUT}
  DESTINATION "${CMAKE_BINARY_DIR}/bin/Debug/translations")

install(FILES ${TRANSLATIONS_OUT}
  DESTINATION "bin/translations"
  COMPONENT Client)

install(TARGETS client_qt
  RUNTIME DESTINATION "bin"
  COMPONENT Client)

# UT-s

add_executable(client_qt_unittests ${UNITTESTS} ${UNITTESTS_QT})
# Uses the custom runner from `test/qt/test_main_qt.cpp`.
target_link_libraries(client_qt_unittests PRIVATE client_qt_lib base_unittest)
include(GoogleTest)
gtest_discover_tests(client_qt_unittests)
