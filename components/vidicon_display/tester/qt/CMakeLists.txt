file(GLOB VIDICON_DISPLAY_TESTER_SOURCES CONFIGURE_DEPENDS
  "*.cpp"
  "*.h"
)

add_executable(vidicon_display_tester WIN32 ${VIDICON_DISPLAY_TESTER_SOURCES})

target_link_libraries(vidicon_display_tester PRIVATE
  client_base_qt
  client_vidicon_display_qt
  client_vidicon_module
  display_tester
)
