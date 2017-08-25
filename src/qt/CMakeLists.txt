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

ADD_EXECUTABLE(client_qt WIN32 ${sources})

TARGET_LINK_LIBRARIES(client_qt ${libs})

SET_PROPERTY(TARGET client_qt APPEND PROPERTY COMPILE_FLAGS "-DUI_QT")
SET_PROPERTY(TARGET client_qt PROPERTY FOLDER ${client_folder})

# UT-s

# ADD_EXECUTABLE(client_unittests_qt WIN32 ${unittests})

# TARGET_LINK_LIBRARIES(client_unittests_qt ${libs} gmock_main)
# SET_PROPERTY(TARGET client_unittests_qt APPEND PROPERTY COMPILE_FLAGS "-DUI_QT")
# SET_PROPERTY(TARGET client_unittests_qt PROPERTY FOLDER ${client_folder})
