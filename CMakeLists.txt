include_directories(client
  "."
  "${third_party}/modus"
  "${third_party}/opc"
)

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
