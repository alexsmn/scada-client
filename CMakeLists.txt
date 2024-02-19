# Required for `AUTOMOC`, `AUTOUIC`, and `AUTORCC` added to all
# `client_module` targets.
find_package(Qt5 COMPONENTS Widgets REQUIRED)
find_package(Qt5LinguistTools REQUIRED)

include(client_module.cmake)
include(translation.cmake)

include_directories(client
  "."
  "${third_party}/opc"
)

add_subdirectory(base)
add_subdirectory(aui)
add_subdirectory(clipboard)
add_subdirectory(profile)
# `controller` depends on `profile`.
add_subdirectory(controller)
add_subdirectory(services)
add_subdirectory(portfolio)
add_subdirectory(filesystem)
add_subdirectory(favorites)
add_subdirectory(web)
add_subdirectory(print)
add_subdirectory(properties)
add_subdirectory(modus)
add_subdirectory(vidicon)
add_subdirectory(configuration_tree)
add_subdirectory(graph)
add_subdirectory(export)
add_subdirectory(test)
add_subdirectory(main_window)

file(GLOB_RECURSE PATHS CONFIGURE_DEPENDS
  "*.cpp"
  "*.h"
  "*.rc"
  "*.ui"
)

list(FILTER PATHS EXCLUDE REGEX "/client/app/")
list(FILTER PATHS EXCLUDE REGEX "/client/aui/")
list(FILTER PATHS EXCLUDE REGEX "/client/base/")
list(FILTER PATHS EXCLUDE REGEX "/client/clipboard/")
list(FILTER PATHS EXCLUDE REGEX "/client/configuration_tree/")
list(FILTER PATHS EXCLUDE REGEX "/client/controller/")
list(FILTER PATHS EXCLUDE REGEX "/client/export/")
list(FILTER PATHS EXCLUDE REGEX "/client/favorites/")
list(FILTER PATHS EXCLUDE REGEX "/client/filesystem/")
list(FILTER PATHS EXCLUDE REGEX "/client/graph/")
list(FILTER PATHS EXCLUDE REGEX "/client/main_window/")
list(FILTER PATHS EXCLUDE REGEX "/client/modus/")
list(FILTER PATHS EXCLUDE REGEX "/client/portfolio/")
list(FILTER PATHS EXCLUDE REGEX "/client/print/")
list(FILTER PATHS EXCLUDE REGEX "/client/profile/")
list(FILTER PATHS EXCLUDE REGEX "/client/properties/")
list(FILTER PATHS EXCLUDE REGEX "/client/services/")
list(FILTER PATHS EXCLUDE REGEX "/client/vidicon/")
# Exclude `main.cpp` from all executables: apps and tests.
list(FILTER PATHS EXCLUDE REGEX "main.cpp")

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

add_subdirectory(components)
add_subdirectory(app)
