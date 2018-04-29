file(GLOB_RECURSE PATHS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
  "*.cpp"
  "*.h"
  "*.rc"
  "*.ui"
)

list(REMOVE_ITEM PATHS
  "actions.cpp"
  "actions.h"
)

foreach(PATH ${PATHS})
  IF(${PATH} MATCHES "_unittest")
    IF(${PATH} MATCHES "/views/")
      LIST(APPEND MAIN_COMPONENT_UNITTESTS_VIEWS ${PATH})
    ELSEIF(${PATH} MATCHES "/qt/")
      LIST(APPEND MAIN_COMPONENT_UNITTESTS_QT ${PATH})
    ELSE()
      LIST(APPEND MAIN_COMPONENT_UNITTESTS ${PATH})
    ENDIF()
  ELSE()
    IF(${PATH} MATCHES "/views/")
      LIST(APPEND MAIN_COMPONENT_SOURCES_VIEWS ${PATH})
    ELSEIF(${PATH} MATCHES "/qt/")
      LIST(APPEND MAIN_COMPONENT_SOURCES_QT ${PATH})
    ELSE()
      LIST(APPEND MAIN_COMPONENT_SOURCES ${PATH})
    ENDIF()
  ENDIF()
endforeach()

add_library(main_component_qt ${MAIN_COMPONENT_SOURCES} ${MAIN_COMPONENT_SOURCES_QT})
target_link_libraries(main_component_qt PUBLIC base)
set_property(TARGET main_component_qt PROPERTY FOLDER ${client_folder})

add_executable(main_component_qt_unittest WIN32 ${MAIN_COMPONENT_UNITTESTS} ${MAIN_COMPONENT_UNITTESTS_QT})
target_link_libraries(main_component_qt_unittest PUBLIC main_component_qt)
set_property(TARGET main_component_qt_unittest PROPERTY FOLDER ${client_folder})
