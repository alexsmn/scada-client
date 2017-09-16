set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Automatically link Qt executables to qtmain target on Windows
cmake_policy(SET CMP0020 NEW)

FIND_PACKAGE(Qt5LinguistTools)
FIND_PACKAGE(Qt5Widgets)

ADD_SUBDIRECTORY("${deps}/qt/graph_qt" "graph_qt")

LIST(APPEND sources ${sources_qt})
LIST(APPEND unittests ${unittests_qt})

LIST(APPEND libs
  graph_qt
  Qt5::Widgets
  ui_base
)

IF(WIN32)
  FIND_PACKAGE(Qt5WinExtras)
  LIST(APPEND libs Qt5::WinExtras)
ENDIF()

file(GLOB translations "*.ts")
qt5_create_translation(translations_out ${sources} ${translations})
qt5_add_translation(translations_out ${translations})

ADD_EXECUTABLE(client_qt WIN32 ${sources} ${translations} ${translations_out})

TARGET_LINK_LIBRARIES(client_qt ${libs})

SET_PROPERTY(TARGET client_qt APPEND PROPERTY COMPILE_FLAGS "-DUI_QT")
SET_PROPERTY(TARGET client_qt PROPERTY FOLDER ${client_folder})

# UT-s

# ADD_EXECUTABLE(client_unittests_qt WIN32 ${unittests})

# TARGET_LINK_LIBRARIES(client_unittests_qt ${libs} gmock_main)
# SET_PROPERTY(TARGET client_unittests_qt APPEND PROPERTY COMPILE_FLAGS "-DUI_QT")
# SET_PROPERTY(TARGET client_unittests_qt PROPERTY FOLDER ${client_folder})
