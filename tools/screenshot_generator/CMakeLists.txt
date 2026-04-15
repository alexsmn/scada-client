# Offline screenshot generator: renders scada-docs/img/* from a JSON
# fixture. It should ride the same client-side dependency stack as the
# real Qt application, plus the shared gtest runner used by the Qt test
# binaries. `client_application.{cpp,h}` is excluded from `client_qt_lib`
# (see app/qt/CMakeLists.txt) so it's compiled in directly here — same
# arrangement as the regular `client_qt` exe.

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
  client_qt_lib
  base_unittest
  graph_qt
)

add_dependencies(screenshot_generator client_qt_copy_translations)

set_target_properties(screenshot_generator PROPERTIES
  AUTOMOC ON
)

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
