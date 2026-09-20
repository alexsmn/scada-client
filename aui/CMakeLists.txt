# The abstract UI layer: platform-agnostic models plus the Qt view
# wrappers. Slated for extraction into its own repository, so this file works
# both as a subdirectory of the client build (through
# FindScadaClientAui.cmake) and as a standalone top-level project. See
# docs/client/aui-extraction.md for the dependency contract.

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  cmake_minimum_required(VERSION 3.25)
  project(scada-client-aui VERSION 2.6.0 LANGUAGES CXX)

  set(CMAKE_CXX_STANDARD 23)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)

  enable_testing()
endif()

# UI configuration: a consuming project (the client root) defines
# CLIENT_UI_CONFIGS before adding aui; derive it from BUILD_CLIENT_QT when
# building standalone. Qt is the only flavour — the Wt one was deleted, see
# the client root CMakeLists.
if(NOT DEFINED CLIENT_UI_CONFIGS)
  option(BUILD_CLIENT_QT "Build the Qt flavor (aui_qt)" ON)

  set(CLIENT_UI_CONFIGS)
  if(BUILD_CLIENT_QT)
    list(APPEND CLIENT_UI_CONFIGS qt)
  endif()
  if(NOT CLIENT_UI_CONFIGS)
    message(FATAL_ERROR "At least one aui UI flavor must be enabled")
  endif()
endif()

# The scada_module()/facade machinery and scada_base come from scada-core; in
# the superproject and client builds both are already present.
if(NOT COMMAND scada_module OR NOT TARGET scada_base)
  find_package(ScadaCore REQUIRED)
endif()

# aui owns the flavoured _qt target helpers (client_module() and friends); the
# client repo and any other consumer reuse them from here.
if(NOT COMMAND client_module)
  include("${CMAKE_CURRENT_LIST_DIR}/client_module.cmake")
endif()

# client_module() enables AUTOMOC/AUTOUIC/AUTORCC on Qt targets, so Qt must be
# found before target creation when building standalone.
if("qt" IN_LIST CLIENT_UI_CONFIGS)
  find_package(Qt6 REQUIRED COMPONENTS Widgets)
  find_package(lunasvg CONFIG REQUIRED)
endif()

client_module(aui)
client_module_sources(aui PRIVATE models)

# aui headers are included as "aui/<header>", so export the parent of this
# directory (the graph_qt pattern). The checkout directory must therefore be
# named `aui`; in the client tree it already is.
client_module_include_directories(aui PUBLIC "..")

# Keep this link set minimal and honest: aui is the platform-agnostic UI
# layer, slated for extraction into its own repository. Its project
# dependencies are scada_base (core/base utilities) and the standalone view
# component libraries (graph_qt, view_manager_qt — declared below for the Qt
# flavor); everything else it needs is Qt (aui_qt). Do not add
# links to client-repo modules here — invert the dependency instead (see
# docs/client/aui-extraction.md).
client_module_link_libraries(aui PUBLIC
  scada_base
)

# QT

if(TARGET aui_qt)
  target_compile_definitions(aui_qt PUBLIC UI_QT)

  target_compile_features(aui_qt PUBLIC cxx_std_23)

  target_link_libraries(aui_qt PUBLIC
    # Lucide row glyphs are rasterised by lunasvg rather than Qt6::Svg:
    # qtsvg's vcpkg build requires full Xcode, and this was the last
    # dependency keeping the client from building standalone.
    lunasvg::lunasvg
    Qt6::Widgets
  )

  if(WIN32)
    find_package(Qt6 COMPONENTS AxContainer REQUIRED)
    target_link_libraries(aui_qt PUBLIC
      Qt6::AxContainer
    )
  endif()

  # Abstract-UI seams over the standalone view component libraries:
  # aui/qt/graph.h wraps graph_qt, aui/view_manager.h wraps
  # view_manager_qt. Declared here so aui owns the include paths its own
  # headers use instead of borrowing them from consumer targets
  # (client_graph / main_window). Both find modules are resolved through the
  # preset-seeded CMAKE_MODULE_PATH.
  find_package(GraphQt REQUIRED)
  find_package(ViewManagerQt REQUIRED)
  target_link_libraries(aui_qt PUBLIC
    graph_qt
    view_manager_qt
  )

  # C++20 module facade.
  scada_add_module_facade(aui_qt FILES scada_client_aui.cppm
    IMPORTS scada_base_module)
  if(SCADA_CXX_MODULES)
    add_subdirectory(module_test)
  endif()
endif()

# `QAbstractItemModelTester` lives in Qt Test, and the model adapters are
# exactly what it exists to check -- it asserts the whole
# begin/end-insert-remove-reset contract, including cases nobody writes a
# bespoke assertion for. Linked into the test executable only, never into
# `aui_qt`, so nothing of Qt Test ships in the client.
if(TARGET aui_qt_unittests)
  find_package(Qt6 REQUIRED COMPONENTS Test)
  target_link_libraries(aui_qt_unittests PRIVATE Qt6::Test)
endif()

# Subs.

add_subdirectory(test)
