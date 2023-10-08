file(GLOB MODUS_DISPLAY_TESTER_SOURCES CONFIGURE_DEPENDS
  "*.cpp"
  "*.h"
)

add_executable(modus_display_tester WIN32 ${MODUS_DISPLAY_TESTER_SOURCES})

target_link_libraries(modus_display_tester PRIVATE
  client_base_qt
  client_controller_qt
  client_filesystem
  client_modus_qt
  # TODO: Cleanup dependencies.
  # client_qt_lib
  display_tester
)
