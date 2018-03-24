include_directories(client
  "."
  "${third_party}/modus"
  "${third_party}/opc"
)

set(WTL_INCLUDE_DIR "${deps}/wtl/include")

add_subdirectory("${deps}/chromeui" chromeui)
add_subdirectory("${deps}/modus" modus)

add_definitions(
  -DMODUS_GDIP)

file(GLOB CORE_SOURCES
  "base/*.cpp"
  "base/*.h"
  "services/alias_service.cpp"
  "services/alias_service.h"
  "services/excel_configuration_commands.cpp"
  "services/excel_configuration_commands.h"
  "services/import_export.cpp"
  "services/import_export.h"
)
list(APPEND CORE_SOURCES
  "${CMAKE_CURRENT_SOURCE_DIR}/components/configuration_tree/configuration_tree_model.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/components/configuration_tree/configuration_tree_model.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/components/events/event_table_model.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/components/events/event_table_model.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/components/favourites/favourites_tree_model.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/components/favourites/favourites_tree_model.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/components/login/login_controller.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/components/login/login_controller.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/components/main/action.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/components/main/action.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/components/object_tree/object_tree_model.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/components/object_tree/object_tree_model.h"
  # "${CMAKE_CURRENT_SOURCE_DIR}/components/main/main_window.cpp"
  # "${CMAKE_CURRENT_SOURCE_DIR}/components/main/main_window.h"
)

file(GLOB CORE_UT_SOURCES
  "base/*_unittest.*"
  "services/alias_service_unittest.cpp"
  "services/excel_configuration_commands_unittest.cpp"
)

list(REMOVE_ITEM CORE_SOURCES ${CORE_UT_SOURCES})

add_library(client_core ${CORE_SOURCES})
set_property(TARGET client_core PROPERTY FOLDER ${client_folder})

target_link_libraries(client_core PUBLIC
  address_space_common
  base_i18n
  modus
  scada_core_remote
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

file(GLOB CLIENT_UT_SOURCES
  "components/configuration_tree/*_unittest.cpp"
  "components/events/*_unittest.cpp"
  "components/graph/*_unittest.cpp"
  "components/hardware_tree/*_unittest.cpp"
  "components/node_table/*_unittest.h"
  "components/object_tree/*_unittest.cpp"
  "components/table/*_unittest.cpp"
  "models/*_unittest.cpp"
  "services/*_unittest.cpp"
)

add_executable(client_core_unittests ${CORE_UT_SOURCES})
target_link_libraries(client_core_unittests PUBLIC client_core gmock_main)
add_test(NAME client_core_unittests COMMAND client_core_unittests)
set_property(TARGET client_core_unittests PROPERTY FOLDER ${client_folder})

add_subdirectory(views)
add_subdirectory(qt)
