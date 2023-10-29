# Automatically link Qt executables to qtmain target on Windows
cmake_policy(SET CMP0020 NEW)

file(GLOB MAIN_QT
  "../*.rc"
  "../app/qt/main.cpp"
  "../res/client.qrc"
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

target_link_libraries(client_qt_lib PUBLIC
  aui_qt
  client_common
  client_controller_qt
  client_graph_qt
  client_main_window_qt
  client_modus_qt
  client_portfolio_qt
  client_print_preview_qt
  client_print_service_qt
  client_vidicon_display_qt
)

find_package(Qt5LinguistTools REQUIRED)
file(GLOB TRANSLATIONS CONFIGURE_DEPENDS "*.ts")
qt5_create_translation(TRANSLATIONS_OUT ${SOURCES_QT} ${MAIN_QT} ${TRANSLATIONS})
qt5_add_translation(TRANSLATIONS_OUT ${TRANSLATIONS})
target_sources(client_qt_lib PRIVATE ${TRANSLATIONS} ${TRANSLATIONS_OUT})

# Exe

add_executable(client_qt WIN32 ${MAIN_QT})
target_link_libraries(client_qt PUBLIC client_qt_lib)
set_property(TARGET client_qt PROPERTY OUTPUT_NAME "client")

install(FILES ${TRANSLATIONS_OUT} DESTINATION "bin/translations" COMPONENT Client)

install(TARGETS client_qt
  RUNTIME DESTINATION "bin"
  COMPONENT Client)

# UT-s

add_executable(client_qt_unittests ${UNITTESTS} ${UNITTESTS_QT})
# Uses the custom runner from `test/qt/test_main_qt.cpp`.
target_link_libraries(client_qt_unittests PUBLIC client_qt_lib base_unittest)
include(GoogleTest)
gtest_discover_tests(client_qt_unittests)
