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
#
# `display` is NOT among them, and that is the whole of ADR 0013: the schematic
# renderer's SOURCE is never published, so the client loads it as a shared
# library at run time instead of compiling it. What the client needs from that
# product at BUILD time is one C header, resolved below — no sources, no
# targets, no link. The renderer's BINARY does publish, as a release asset, so
# "never published" on its own is the half of the sentence that inverts it.
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

# ── The display runtime's ABI header ────────────────────────────────────────
#
# `client_display_view` loads the schematic renderer through
# display/abi/display_abi.h (ADR 0013). The header is `display`'s and stays
# there — one copy in the monorepo — so where it is found differs by layout,
# while the #include spelling does not:
#
#   tree    the resolver finds the display product and the header is under it
#   export  tools/export/products.toml places the one header at this root,
#           which is already on the include path
#
# QUIET, and no find_package: this resolves a DIRECTORY, never a target. Adding
# display as a product again would put its sources back in the client's closure
# and undo the reason this ADR exists.
scada_resolve_product(display SCADA_DISPLAY_PRODUCT_DIR QUIET)

add_library(client_display_abi INTERFACE)
if(SCADA_DISPLAY_PRODUCT_DIR)
  target_include_directories(client_display_abi
    INTERFACE "${SCADA_DISPLAY_PRODUCT_DIR}")
elseif(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/display/abi/display_abi.h")
  message(FATAL_ERROR
    "display/abi/display_abi.h was not found. In the monorepo it comes from "
    "the `display` product; in an export it is carried at this product's root "
    "by tools/export/products.toml. Neither was there.")
endif()

# ── Where the runtime binary comes from ────────────────────────────────────
#
# Nowhere, by default, and that is a supported state: a client with no runtime
# runs and reports "no display runtime" where a schematic would be (ADR 0013
# design point 3). It is not a build configuration — the display SURFACE is
# always compiled in.
#
# Naming the binary by path rather than building it follows ADR 0011, the same
# way the E2E suite names its tier binaries: `display` is a separate product
# with its own build tree and there is no shared bin/. Build it there, then
# either pass the path here to have it staged beside the client, or set
# SCADA_DISPLAY_RUNTIME in the environment at run time.
set(SCADA_DISPLAY_RUNTIME_LIBRARY "" CACHE FILEPATH
    "The built display runtime shared library, staged beside the client")
if(SCADA_DISPLAY_RUNTIME_LIBRARY AND NOT EXISTS "${SCADA_DISPLAY_RUNTIME_LIBRARY}")
  # A path that is set but wrong is a mistake, where absence is a mode.
  message(FATAL_ERROR
    "SCADA_DISPLAY_RUNTIME_LIBRARY points at "
    "'${SCADA_DISPLAY_RUNTIME_LIBRARY}', which does not exist.")
endif()
if(NOT SCADA_DISPLAY_RUNTIME_LIBRARY)
  message(STATUS
    "client: no display runtime staged; schematic windows will report that "
    "there is none. To have one, build the `display` product and configure "
    "with\n"
    "    -DSCADA_DISPLAY_RUNTIME_LIBRARY=<display>/build/ninja/bin/<Config>/"
    "libdisplay_runtime.<so|dylib|dll>")
endif()

# client_stage_display_runtime(<executable> [DIRECTORY <dir>])
#
# Copies the configured runtime next to `<executable>`, which is where
# `DisplayRuntime::SearchPaths()` looks first. Does nothing when no runtime was
# configured -- that is a supported state, not a build error.
#
# DIRECTORY overrides the destination, for a target whose executable does not
# sit directly in the runtime output directory: a macOS .app puts it inside the
# bundle.
#
# Driven by the LIBRARY rather than by the link. A POST_BUILD command fires only
# when the target relinks, so rebuilding the runtime and then building the
# client would leave the stale copy in place and the change would be invisible
# to the running app -- the same trap the bundle translations documented after
# it cost a day.
function(client_stage_display_runtime target)
  if(NOT SCADA_DISPLAY_RUNTIME_LIBRARY)
    return()
  endif()
  cmake_parse_arguments(_stage "" "DIRECTORY" "" ${ARGN})

  set(_dir "${_stage_DIRECTORY}")
  if(NOT _dir)
    set(_dir "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
  endif()

  get_filename_component(_name "${SCADA_DISPLAY_RUNTIME_LIBRARY}" NAME)
  set(_staged "${_dir}/${_name}")

  add_custom_command(
    OUTPUT "${_staged}"
    DEPENDS "${SCADA_DISPLAY_RUNTIME_LIBRARY}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${_dir}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "${SCADA_DISPLAY_RUNTIME_LIBRARY}" "${_dir}"
    COMMENT "Stage the display runtime for ${target}")

  add_custom_target(${target}_stage_display_runtime DEPENDS "${_staged}")
  add_dependencies(${target} ${target}_stage_display_runtime)
endfunction()

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
