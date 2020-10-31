# Automatically link Qt executables to qtmain target on Windows
cmake_policy(SET CMP0020 NEW)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_subdirectory("${deps}/qt/graph_qt" "graph_qt") 
add_subdirectory("${deps}/qt/schematic" "schematic") 

add_executable(client_qt WIN32
  ${SOURCES}
  ${SOURCES_QT}
  "../res/client.qrc"
)

set_property(TARGET client_qt PROPERTY FOLDER ${client_folder})

target_compile_definitions(client_qt PUBLIC
  "-DUI_QT"
)

find_package(Qt5 COMPONENTS Core Widgets PrintSupport REQUIRED)
target_link_libraries(client_qt PUBLIC
  address_space_common
  base_i18n
  gdiplus
  graph_qt
  modus
  Qt5::Widgets
  Qt5::PrintSupport
  remote_node_service
  scada_core_opcua
  scada_core_remote
  scada_core_vidicon
  schematic
  ui_base
)

if(WIN32)
  find_package(Qt5 COMPONENTS AxContainer WinExtras REQUIRED)
  target_link_libraries(client_qt PUBLIC
    Qt5::AxContainer
    Qt5::WinExtras
    Qt5::WinMain
  )
endif()

find_package(Qt5LinguistTools REQUIRED)
file(GLOB TRANSLATIONS "*.ts")
qt5_create_translation(TRANSLATIONS_OUT ${SOURCES_QT} ${TRANSLATIONS})
qt5_add_translation(TRANSLATIONS_OUT ${TRANSLATIONS})
target_sources(client_qt PUBLIC ${TRANSLATIONS} ${TRANSLATIONS_OUT})

set_property(TARGET client_qt PROPERTY OUTPUT_NAME "client")

install(TARGETS client_qt
  RUNTIME DESTINATION "bin"
  COMPONENT Client)

install(FILES ${TRANSLATIONS_OUT} DESTINATION "bin/translations" COMPONENT Client)

# UT-s

# LIST(APPEND client_unittests_qt_sources ${CLIENT_SOURCES} ${client_qt_sources} ${CLIENT_UT_SOURCES})
# LIST(REMOVE_ITEM client_unittests_qt_sources "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")

# ADD_EXECUTABLE(client_qt_unittests WIN32 ${client_unittests_qt_sources})
# target_link_libraries(client_qt_unittests PUBLIC ${client_qt_libs} gmock_main)
# SET_PROPERTY(TARGET client_qt_unittests APPEND PROPERTY COMPILE_FLAGS "-DUI_QT")

# SET_PROPERTY(TARGET client_qt_unittests PROPERTY FOLDER ${client_folder})
