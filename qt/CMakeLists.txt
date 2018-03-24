set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Automatically link Qt executables to qtmain target on Windows
cmake_policy(SET CMP0020 NEW)

add_subdirectory("${deps}/qt/graph_qt" "graph_qt") 

add_executable(client_qt WIN32
  ${SOURCES}
  ${SOURCES_QT}
)

set_property(TARGET client_qt PROPERTY FOLDER ${client_folder})

target_compile_definitions(client_qt PUBLIC
  "-DUI_QT"
)

find_package(Qt5 COMPONENTS Core Widgets REQUIRED)
target_link_libraries(client_qt PUBLIC
  client_core
  gdiplus
  graph_qt
  Qt5::Widgets
)

if(WIN32)
  find_package(Qt5 COMPONENTS WinExtras)
  target_link_libraries(client_qt PUBLIC
    Qt5::WinExtras
    Qt5::WinMain
  )
endif()

find_package(Qt5LinguistTools REQUIRED)
file(GLOB TRANSLATIONS "*.ts")
qt5_create_translation(translations_out ${SOURCES} ${TRANSLATIONS})
qt5_add_translation(TRANSLATIONS_OUT ${TRANSLATIONS})
target_sources(client_qt PUBLIC ${TRANSLATIONS})

# UT-s

# ADD_EXECUTABLE(client_unittests_qt WIN32 ${unittests})

# TARGET_LINK_LIBRARIES(client_unittests_qt ${libs} gmock_main)
# SET_PROPERTY(TARGET client_unittests_qt APPEND PROPERTY COMPILE_FLAGS "-DUI_QT")
# SET_PROPERTY(TARGET client_unittests_qt PROPERTY FOLDER ${client_folder})
