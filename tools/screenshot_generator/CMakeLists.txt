# Offline screenshot generator: renders scada-docs/img/* from a JSON
# fixture. Links against the smaller client_qt_core runtime plus
# client_main_window_qt for the actual top-level shell, and base_unittest
# for the shared gtest main. `client_application.{cpp,h}` is excluded
# from client_qt_core (see app/qt/CMakeLists.txt) so it's compiled in
# directly here — same arrangement as the regular `client_qt` exe.

add_executable(screenshot_generator
  "main.cpp"
  "screenshot_config.cpp"
  "screenshot_config.h"
  "screenshot_output.cpp"
  "screenshot_output.h"
  "widget_capture.cpp"
  "widget_capture.h"
  "graph_capture.cpp"
  "graph_capture.h"
  "dialog_capture.cpp"
  "dialog_capture.h"
  "fixture_builder.cpp"
  "fixture_builder.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/../../app/client_application.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/../../app/client_application.h"
)

target_link_libraries(screenshot_generator PRIVATE
  client_qt_core
  client_main_window_qt
  client_properties_qt
  base_unittest
  graph_qt
)

set_target_properties(screenshot_generator PROPERTIES
  AUTOMOC ON
)

# Lighten the linker's memory footprint. The 32-bit Microsoft link.exe
# otherwise hits LNK1102 ("out of memory") on this target: it links
# the Qt runtime, main-window shell, and gtest inside the 4 GB
# virtual-address ceiling of a 32-bit process. /DEBUG:NONE drops the
# 600 MB+ PDB; /INCREMENTAL:NO drops the .ilk bookkeeping; /OPT:REF +
# /OPT:ICF let the linker throw away unreferenced symbols and fold
# identical functions (saves a lot on template-heavy input). This is
# a tooling binary that we never attach a debugger to, so losing its
# PDB is fine — the client exe is unaffected.
if(MSVC)
  target_link_options(screenshot_generator PRIVATE
    "/DEBUG:NONE"
    "/INCREMENTAL:NO"
    "/OPT:REF"
    "/OPT:ICF"
  )
endif()

# After the executable links, run it and drop the PNGs into
# client/docs/screenshots/ so the docs gallery stays in sync on every
# local build. The dir is gitignored — commit copies to scada-docs
# separately. Requires a desktop session (see docs/screenshots.md
# "Native window frames").
set(SCREENSHOT_DOCS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../docs/screenshots")
add_custom_command(TARGET screenshot_generator POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory "${SCREENSHOT_DOCS_DIR}"
  COMMAND ${CMAKE_COMMAND} -E env
    "SCREENSHOT_OUT_DIR=${SCREENSHOT_DOCS_DIR}"
    $<TARGET_FILE:screenshot_generator>
  COMMENT "Rendering screenshots into docs/screenshots/"
  VERBATIM
)
