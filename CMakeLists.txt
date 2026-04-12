cmake_minimum_required(VERSION 3.25)

project(scada-client
  VERSION 2.5.6
  LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32)
  add_definitions(
    -D_MSVC_STL_HARDENING
  )
endif()

enable_testing()

# Required for `AUTOMOC`, `AUTOUIC`, and `AUTORCC` added to all
# `client_module` targets.
find_package(Qt5 COMPONENTS Widgets REQUIRED)
find_package(Qt5LinguistTools REQUIRED)

include(client_module.cmake)
include(translation.cmake)

find_package(ScadaCommon REQUIRED)

include_directories(client
  "."
)

add_subdirectory(base)
add_subdirectory(core)
add_subdirectory(aui)
add_subdirectory(clipboard)
add_subdirectory(profile)
# `controller` depends on `profile`.
add_subdirectory(controller)
add_subdirectory(services)
add_subdirectory(events)
add_subdirectory(portfolio)
add_subdirectory(filesystem)
add_subdirectory(favorites)
add_subdirectory(web)
add_subdirectory(print)
add_subdirectory(properties)
option(BUILD_MODUS "Build Modus module (requires Modus and Schematic SDKs)" ON)

if(BUILD_MODUS)
  add_subdirectory(modus)
endif()

# vidicon depends on scada_common_opc and scada_common_vidicon.
if(TARGET scada_common_opc AND TARGET scada_common_vidicon)
  add_subdirectory(vidicon)
endif()

add_subdirectory(configuration)
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

list(FILTER PATHS EXCLUDE REGEX "/build/")
list(FILTER PATHS EXCLUDE REGEX "/client/app/")
list(FILTER PATHS EXCLUDE REGEX "/client/aui/")
list(FILTER PATHS EXCLUDE REGEX "/client/base/")
list(FILTER PATHS EXCLUDE REGEX "/client/clipboard/")
list(FILTER PATHS EXCLUDE REGEX "/client/configuration/")
list(FILTER PATHS EXCLUDE REGEX "/client/controller/")
list(FILTER PATHS EXCLUDE REGEX "/client/core/")
list(FILTER PATHS EXCLUDE REGEX "/client/events/")
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
# `tools/` hosts standalone utility executables (e.g. the screenshot
# generator); each manages its own sources via its own CMakeLists.
list(FILTER PATHS EXCLUDE REGEX "/client/tools/")
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
add_subdirectory(tools)
