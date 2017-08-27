SET(MODUS_ROOT_DIR "${deps}/modus")
ADD_SUBDIRECTORY(${MODUS_ROOT_DIR} "modus")
INSTALL(FILES "${MODUS_ROOT_DIR}/libmodus/Library.txt"
  DESTINATION data
  COMPONENT Client)

INCLUDE_DIRECTORIES(client
  "."
  "${deps}/OPCFoundation"
  "${deps}/wtl"
  "${deps}/skia/include"
)

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

FILE(GLOB core_sources
  "*.cpp"
  "*.h"
  "base/*.cpp"
  "base/*.h"
  "services/*.*"
  "services/vidicon/*.*"
  "services/opcua/*.*"
  "services/scada/*.*"
)
LIST(APPEND core_sources
  "${CMAKE_CURRENT_SOURCE_DIR}/components/configuration_tree/configuration_tree_model.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/components/configuration_tree/configuration_tree_model.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/components/events/event_table_model.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/components/events/event_table_model.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/components/favourites/favourites_tree_model.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/components/favourites/favourites_tree_model.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/components/main/action.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/components/main/action.h"
  # "${CMAKE_CURRENT_SOURCE_DIR}/components/main/view_manager.cpp"
  # "${CMAKE_CURRENT_SOURCE_DIR}/components/main/view_manager.h"
  # "${CMAKE_CURRENT_SOURCE_DIR}/components/main/view_manager_delegate.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/components/object_tree/object_tree_model.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/components/object_tree/object_tree_model.h"
)
LIST(REMOVE_ITEM core_sources
  "${CMAKE_CURRENT_SOURCE_DIR}/controller.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/controller_delegate.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/controller_factory.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/controller_factory.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/dialog_service.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/base/color.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/base/color.h"
)

ADD_LIBRARY(client_core STATIC ${core_sources})
TARGET_LINK_LIBRARIES(client_core
  base_i18n
  ui_base
  scada_common
  modus
  remote
)
SET_PROPERTY(TARGET client_core PROPERTY FOLDER ${client_folder})

IF(WIN32)
  TARGET_LINK_LIBRARIES(client_core OPCFoundation)
ENDIF()

FILE(GLOB_RECURSE paths
  "*.cpp"
  "*.h"
  "*.rc"
)
LIST(REMOVE_ITEM paths ${core_sources})

FOREACH(path ${paths})
  IF(${path} MATCHES "_unittest")
    IF(${path} MATCHES "/views/")
      LIST(APPEND unittests_views ${path})
    ELSEIF(${path} MATCHES "/qt/")
      LIST(APPEND unittests_qt ${path})
    ELSE()
      LIST(APPEND unittests ${path})
    ENDIF()
  ELSE()
    IF(${path} MATCHES "/views/")
      LIST(APPEND sources_views ${path})
    ELSEIF(${path} MATCHES "/qt/")
      LIST(APPEND sources_qt ${path})
    ELSE()
      LIST(APPEND sources ${path})
    ENDIF()
  ENDIF()
ENDFOREACH()

LIST(APPEND libs client_core)

ADD_SUBDIRECTORY(views)
ADD_SUBDIRECTORY(qt)
