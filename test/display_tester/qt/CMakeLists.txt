scada_module(display_tester)

find_package(Qt5 COMPONENTS Core Widgets REQUIRED)

target_link_libraries(display_tester PUBLIC
  client_qt_lib
)
