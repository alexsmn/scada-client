# Automatically link Qt executables to qtmain target on Windows
cmake_policy(SET CMP0020 NEW)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_subdirectory("${deps}/qt/graph_qt" "graph_qt") 
add_subdirectory("${deps}/qt/schematic" "schematic") 

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

target_compile_definitions(client_qt_lib PUBLIC
  "-DUI_QT"
)

set_property(TARGET client_qt_lib PROPERTY FOLDER ${client_folder})

find_package(Qt5 COMPONENTS Core Widgets PrintSupport REQUIRED)
target_link_libraries(client_qt_lib PUBLIC
  base_i18n
  gdiplus
  graph_qt
  modus
  Qt5::Widgets
  Qt5::PrintSupport
  node_service_v1
  node_service_v2
  scada_core_opcua
  scada_core_remote
  scada_core_vidicon
  schematic
  timed_data
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
set_property(TARGET client_qt PROPERTY FOLDER ${client_folder})
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
set_property(TARGET client_qt_unittests PROPERTY FOLDER ${client_folder})