include_directories(client
  "."
  "${third_party}/modus"
  "${third_party}/opc"
)

set(WTL_INCLUDE_DIR "${deps}/wtl/include")

add_subdirectory("${deps}/chromeui" chromeui)
add_subdirectory("${deps}/modus" modus)

add_definitions(-DMODUS_GDIP)

file(GLOB_RECURSE PATHS CONFIGURE_DEPENDS
  "*.cpp"
  "*.h"
  "*.rc"
  "*.ui"
)

list(FILTER PATHS EXCLUDE REGEX "components/vidicon_display/tester")

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

add_subdirectory(views)
add_subdirectory(qt)
add_subdirectory(components)
