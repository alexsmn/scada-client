include(client_module.cmake)

include_directories(client
  "."
  "${third_party}/modus"
  "${third_party}/opc"
)

add_subdirectory("${deps}/modus" modus)
add_subdirectory(base)
add_subdirectory(services)
add_subdirectory(test)

add_definitions(-DMODUS_GDIP)

add_library(client_common INTERFACE)

target_link_libraries(client_common INTERFACE
  base_i18n
  client_base
  node_service_v1
  node_service_v2
  scada_common_events
  scada_core_opcua
  scada_core_remote
  scada_core_vidicon
  timed_data
  Net::net
)

file(GLOB_RECURSE PATHS CONFIGURE_DEPENDS
  "*.cpp"
  "*.h"
  "*.rc"
  "*.ui"
)

list(FILTER PATHS EXCLUDE REGEX "components/vidicon_display/tester")
list(FILTER PATHS EXCLUDE REGEX "components/modus/tester")
list(FILTER PATHS EXCLUDE REGEX "base")
list(FILTER PATHS EXCLUDE REGEX "services/vidicon")

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

# add_subdirectory(views)
add_subdirectory(qt)
add_subdirectory(wt)
add_subdirectory(components)
