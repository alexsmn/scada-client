# Offline screenshot generator: renders scada-docs/img/* from a JSON
# fixture. Links against client_qt_lib for the UI stack plus base_unittest
# for the shared gtest main. `client_application.{cpp,h}` is excluded
# from client_qt_lib (see app/qt/CMakeLists.txt) so it's compiled in
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
  client_qt_lib
  base_unittest
  graph_qt
)

set_target_properties(screenshot_generator PROPERTIES
  AUTOMOC ON
)
