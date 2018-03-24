CONFIGURE_FILE(
  "${CMAKE_CURRENT_SOURCE_DIR}/project.h.src"
  "${CMAKE_CURRENT_SOURCE_DIR}/project.h"
)

SET(MODUS_ROOT_DIR "${deps}/modus")
ADD_SUBDIRECTORY(${MODUS_ROOT_DIR} "modus")
INSTALL(FILES "${MODUS_ROOT_DIR}/libmodus/Library.txt"
  DESTINATION data
  COMPONENT Client)

include_directories(client
  "."
)

set(WTL_INCLUDE_DIR "${deps}/wtl/include")

ADD_SUBDIRECTORY("${deps}/chromeui" chromeui)
ADD_SUBDIRECTORY(services)

SET_PROPERTY(TARGET ui_base PROPERTY FOLDER ${client_folder})
SET_PROPERTY(TARGET ui_events PROPERTY FOLDER ${client_folder})
SET_PROPERTY(TARGET views PROPERTY FOLDER ${client_folder})
SET_PROPERTY(TARGET skia PROPERTY FOLDER ${client_folder})
SET_PROPERTY(TARGET skia_ext PROPERTY FOLDER ${client_folder})
SET_PROPERTY(TARGET gfx PROPERTY FOLDER ${client_folder})

ADD_DEFINITIONS(
  -DMODUS_GDIP)

file(GLOB CORE_SOURCES
  "*.cpp"
  "*.h"
  "base/*.cpp"
  "base/*.h"
  "services/*.*"
  "services/vidicon/*.*"
  "services/opcua/*.*"
  "services/scada/*.*"
)
list(APPEND CORE_SOURCES
  "${CMAKE_CURRENT_SOURCE_DIR}/components/configuration_tree/configuration_tree_model.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/components/configuration_tree/configuration_tree_model.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/components/object_tree/object_tree_model.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/components/object_tree/object_tree_model.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/components/events/event_table_model.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/components/events/event_table_model.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/components/favourites/favourites_tree_model.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/components/favourites/favourites_tree_model.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/components/main/action.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/components/main/action.h"
  # "${CMAKE_CURRENT_SOURCE_DIR}/components/main/view_manager.cpp"
  # "${CMAKE_CURRENT_SOURCE_DIR}/components/main/view_manager.h"
  # "${CMAKE_CURRENT_SOURCE_DIR}/components/main/view_manager_delegate.h"
)
LIST(REMOVE_ITEM CORE_SOURCES
  "${CMAKE_CURRENT_SOURCE_DIR}/controller.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/controller_delegate.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/controller_factory.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/controller_factory.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/dialog_service.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/base/color.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/base/color.h"
)

add_library(client_core ${CORE_SOURCES})
set_property(TARGET client_core PROPERTY FOLDER ${client_folder})

target_link_libraries(client_core PUBLIC
  base_i18n
  modus
  scada_common
  scada_core_opcua
  scada_core_remote
  scada_core_vidicon
  ui_base
)

file(GLOB_RECURSE PATHS
  "*.cpp"
  "*.h"
  "*.rc"
  "*.ui"
)
list(REMOVE_ITEM PATHS ${CORE_SOURCES})

foreach(PATH ${PATHS})
  IF(${PATH} MATCHES "_unittest")
    IF(${PATH} MATCHES "/views/")
      LIST(APPEND UNITTESTS_VIEWS ${PATH})
    ELSEIF(${PATH} MATCHES "/qt/")
      LIST(APPEND UNITTESTS_QT ${PATH})
    ELSE()
      LIST(APPEND UNITTESTS ${PATH})
    ENDIF()
  ELSE()
    IF(${PATH} MATCHES "/views/")
      LIST(APPEND SOURCES_VIEWS ${PATH})
    ELSEIF(${PATH} MATCHES "/qt/")
      LIST(APPEND SOURCES_QT ${PATH})
    ELSE()
      LIST(APPEND SOURCES ${PATH})
    ENDIF()
  ENDIF()
endforeach()

ADD_SUBDIRECTORY(views)
ADD_SUBDIRECTORY(qt)
