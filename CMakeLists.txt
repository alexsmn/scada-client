cmake_minimum_required(VERSION 3.25)
if(POLICY CMP0167)
  cmake_policy(SET CMP0167 NEW)
endif()

# The shared build-support kit (ADR 0011): at this product's root in an export,
# at the tree root in the monorepo. Before project(), so the machine config it
# reads can still set the vcpkg variables the toolchain consumes there.
set(_scada_dir "${CMAKE_CURRENT_SOURCE_DIR}")
while(NOT EXISTS "${_scada_dir}/build-support/ScadaProducts.cmake")
  get_filename_component(_scada_parent "${_scada_dir}" DIRECTORY)
  if(_scada_parent STREQUAL _scada_dir)
    message(FATAL_ERROR
      "build-support/ not found above ${CMAKE_CURRENT_SOURCE_DIR}")
  endif()
  set(_scada_dir "${_scada_parent}")
endwhile()
include("${_scada_dir}/build-support/ScadaProducts.cmake")
include("${_scada_dir}/build-support/ScadaProductBase.cmake")
unset(_scada_dir)
unset(_scada_parent)

scada_product_prologue()

project(scada-client
  VERSION 2.6.0
  LANGUAGES CXX
)

scada_product_base()

# The products this one consumes. `common` and `opcuapp` are the client's own;
# `core`, `graph_qt` and `view_manager_qt` are aui's — aui is added as a
# subdirectory of this build, so its find_package calls read THIS directory's
# module path, not one of its own.
scada_find_products(common core opcuapp graph_qt view_manager_qt)

enable_testing()

option(BUILD_CLIENT_QT "Build the Qt desktop client" ON)

# Qt is the only UI flavour. The Wt one was deleted — it had not compiled since
# the reshell, had no preset and no CI job, and the shipped web client is the
# separate `web/` workspace. CLIENT_UI_CONFIGS survives the removal because
# client_module() is written against it; it is a one-element list today.
set(CLIENT_UI_CONFIGS)
if(BUILD_CLIENT_QT)
  list(APPEND CLIENT_UI_CONFIGS qt)
endif()
if(NOT CLIENT_UI_CONFIGS)
  message(FATAL_ERROR "At least one client UI must be enabled")
endif()

# Required for `AUTOMOC`, `AUTOUIC`, and `AUTORCC` added to all
# `client_module` targets.
if(BUILD_CLIENT_QT)
  # No Svg component. The qsvgicon icon-engine plugin is only needed when QIcon
  # is asked to load an .svg itself; nothing here does. The tree/table Lucide
  # glyphs (docs/client/ux/iconography.md §4) are rasterised by lunasvg inside aui
  # and reach QIcon as an already-rendered QPixmap, so windeployqt/macdeployqt
  # have no plugin to carry. qtsvg's vcpkg build shells out to `xcodebuild` and
  # needs a full Xcode; it was the last dependency keeping the client from
  # building standalone from its export.
  find_package(Qt6 REQUIRED COMPONENTS Widgets)
  find_package(Qt6 COMPONENTS LinguistTools QUIET)
endif()

find_package(ScadaCommon REQUIRED)

# aui is slated for extraction into its own repository — consume it through
# its find module, which also provides the client_module() dual-target
# helpers used by every client module. The in-tree directory is the fallback
# until the repo split; a sibling checkout seeded earlier on
# CMAKE_MODULE_PATH takes precedence.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/aui")
find_package(ScadaClientAui REQUIRED)

include_directories(client
  "."
  "modules"
)

add_subdirectory(base)
add_subdirectory(core)
add_subdirectory(clipboard)
add_subdirectory(profile)
# `controller` depends on `profile`.
add_subdirectory(controller)
add_subdirectory(services)
add_subdirectory(web)
add_subdirectory(properties)

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
list(FILTER PATHS EXCLUDE REGEX "/client/modules/")
list(FILTER PATHS EXCLUDE REGEX "/client/controller/")
list(FILTER PATHS EXCLUDE REGEX "/client/core/")
list(FILTER PATHS EXCLUDE REGEX "/client/main_window/")
list(FILTER PATHS EXCLUDE REGEX "/client/profile/")
list(FILTER PATHS EXCLUDE REGEX "/client/properties/")
list(FILTER PATHS EXCLUDE REGEX "/client/services/")
list(FILTER PATHS EXCLUDE REGEX "/client/test/e2e/")
# `tools/` hosts standalone utility executables (e.g. the screenshot
# generator); each manages its own sources via its own CMakeLists.
list(FILTER PATHS EXCLUDE REGEX "/client/tools/")
# Exclude `main.cpp` from all executables: apps and tests.
list(FILTER PATHS EXCLUDE REGEX "main.cpp")

foreach(PATH ${PATHS})
  IF(${PATH} MATCHES "_unittest" OR ${PATH} MATCHES "/test/")
    IF(${PATH} MATCHES "/qt/")
      LIST(APPEND UNITTESTS_QT ${PATH})
    ELSE()
      LIST(APPEND UNITTESTS ${PATH})
    ENDIF()
  ELSE()
    IF(${PATH} MATCHES "/qt/")
      LIST(APPEND SOURCES_QT ${PATH})
    ELSE()
      LIST(APPEND SOURCES ${PATH})
    ENDIF()
  ENDIF()
endforeach()

set(CLIENT_COMPONENTS
  about
  change_password
  create
  create_service_item
  debugger
  device_metrics
  limits
  login
  multi_create
  node_properties
  node_table
  select_item
  selection_edit
  sheet
  summary
  table
  time_range
  timed_data
  transmission
  watch
  write
)
if(WIN32)
  list(APPEND CLIENT_COMPONENTS web)
endif()

set(CLIENT_COMPONENT_MODULES)
set(CLIENT_QT_COMPONENT_TARGETS)
foreach(component_name IN LISTS CLIENT_COMPONENTS)
  list(APPEND CLIENT_COMPONENT_MODULES client_component_${component_name})
  list(APPEND CLIENT_QT_COMPONENT_TARGETS client_component_${component_name}_qt)
endforeach()

add_subdirectory(modules)
add_subdirectory(main_window)
add_subdirectory(app)
add_subdirectory(test)
if(BUILD_CLIENT_QT)
  add_subdirectory(tools)
endif()

# Last, because it walks the targets that exist by now: every test executable
# gets the offscreen platform plugin linked in, so the macOS default that
# `aui/test/qt/app_environment.h` applies can actually be honoured by a static
# Qt. See the function's comment in build-support/ScadaProductBase.cmake.
scada_qt_import_offscreen_platform_into_tests()
