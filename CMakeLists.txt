find_package(Qt5 COMPONENTS Core REQUIRED)
# Must be set before Qt targets are created. 
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

include(client_module.cmake)

include_directories(client
  "."
  "${third_party}/opc"
)

add_subdirectory(base)
add_subdirectory(aui)
add_subdirectory(clipboard)
add_subdirectory(controller)
add_subdirectory(filesystem)
add_subdirectory(web)
add_subdirectory(services)
add_subdirectory(vidicon)
add_subdirectory(test)

add_library(client_common INTERFACE)

target_link_libraries(client_common INTERFACE
  base_i18n
  node_service_v1
  node_service_v2
  client_base
  client_clipboard
  scada_common_events
  scada_common_vidicon_services
  scada_core_opcua
  scada_core_remote
  timed_data
  Net::net
)

file(GLOB_RECURSE PATHS CONFIGURE_DEPENDS
  "*.cpp"
  "*.h"
  "*.rc"
  "*.ui"
)

list(FILTER PATHS EXCLUDE REGEX "/client/aui/")
list(FILTER PATHS EXCLUDE REGEX "/client/base/")
list(FILTER PATHS EXCLUDE REGEX "/client/components/graph/")
list(FILTER PATHS EXCLUDE REGEX "/client/components/modus/")
list(FILTER PATHS EXCLUDE REGEX "/client/components/portfolio/")
list(FILTER PATHS EXCLUDE REGEX "/client/clipboard/")
list(FILTER PATHS EXCLUDE REGEX "/client/controller/")
list(FILTER PATHS EXCLUDE REGEX "/client/filesystem/")
list(FILTER PATHS EXCLUDE REGEX "/client/vidicon/")

foreach(PATH ${PATHS})
  IF(${PATH} MATCHES "_unittest" OR ${PATH} MATCHES "/test/")
    IF(${PATH} MATCHES "/qt/")
      LIST(APPEND UNITTESTS_QT ${PATH})
    ELSEIF(${PATH} MATCHES "/wt/")
      LIST(APPEND UNITTESTS_WT ${PATH})
    ELSE()
      LIST(APPEND UNITTESTS ${PATH})
    ENDIF()
  ELSE()
    IF(${PATH} MATCHES "/qt/")
      LIST(APPEND SOURCES_QT ${PATH})
    ELSEIF(${PATH} MATCHES "/wt/")
      LIST(APPEND SOURCES_WT ${PATH})
    ELSE()
      LIST(APPEND SOURCES ${PATH})
    ENDIF()
  ENDIF()
endforeach()

add_subdirectory(qt)
add_subdirectory(wt)
add_subdirectory(components)
