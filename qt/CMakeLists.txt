# Automatically link Qt executables to qtmain target on Windows
cmake_policy(SET CMP0020 NEW)

file(GLOB MAIN_QT
  "../*.rc"
  "../res/client.qrc"
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

find_package(Qt5 COMPONENTS Core Widgets PrintSupport REQUIRED)
target_link_libraries(client_qt_lib PUBLIC
  aui_qt
  client_base_qt
  client_common
  client_controller_qt
  client_graph_qt
  client_modus_qt
  client_portfolio_qt
  client_vidicon_display_qt
  Qt5::PrintSupport
  Qt5::Widgets
)

if(WIN32)
  find_package(Qt5 COMPONENTS AxContainer WinExtras REQUIRED)
  target_link_libraries(client_qt_lib PUBLIC
    Qt5::AxContainer
    Qt5::WinExtras
    Qt5::WinMain
  )
endif()

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
