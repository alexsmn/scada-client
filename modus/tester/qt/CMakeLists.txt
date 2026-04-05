file(GLOB MODUS_DISPLAY_TESTER_SOURCES CONFIGURE_DEPENDS
  "*.cpp"
  "*.h"
)

add_executable(modus_display_tester WIN32 ${MODUS_DISPLAY_TESTER_SOURCES})

target_link_libraries(modus_display_tester PRIVATE
  client_controller_qt
  client_filesystem_qt
  client_modus_qt
  client_portfolio_qt
  client_profile_qt
  display_tester
)
