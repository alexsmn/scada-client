# Offline screenshot generator: renders doc screenshots from a JSON
# fixture. It should ride the same client-side dependency stack as the
# real Qt application, plus the shared gtest runner used by the Qt test
# binaries. `client_application.{cpp,h}` is excluded from `client_qt_lib`
# (see app/qt/CMakeLists.txt) so it's compiled in directly here — same
# arrangement as the regular `client_qt` exe.

add_executable(client_screenshot_generator
  "main.cpp"
  "screenshot_config.cpp"
  "screenshot_config.h"
  "screenshot_options.cpp"
  "screenshot_options.h"
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

target_link_libraries(client_screenshot_generator PRIVATE
  client_qt_lib
  base_unittest
  graph_qt
)

add_dependencies(client_screenshot_generator client_qt_copy_translations)

set_target_properties(client_screenshot_generator PROPERTIES
  AUTOMOC ON
)

# After the executable links, run it and drop the PNGs into
# client/docs/screenshots/ so the local docs gallery stays in sync on
# every build. Requires a desktop session (see docs/screenshots.md
# "Native window frames").
set(SCREENSHOT_DOCS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../docs/screenshots")
add_custom_command(TARGET client_screenshot_generator POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory "${SCREENSHOT_DOCS_DIR}"
  COMMAND $<TARGET_FILE:client_screenshot_generator>
    "--out=${SCREENSHOT_DOCS_DIR}"
    "--image-manifest=${SCREENSHOT_DOCS_DIR}/image_manifest.json"
  COMMENT "Rendering screenshots into docs/screenshots/"
  VERBATIM
)
