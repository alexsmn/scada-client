file(GLOB VIDICON_DISPLAY_TESTER_SOURCES CONFIGURE_DEPENDS
  "*.cpp"
  "*.h"
)

add_executable(vidicon_display_tester WIN32 ${VIDICON_DISPLAY_TESTER_SOURCES})

target_link_libraries(vidicon_display_tester PUBLIC
  display_tester
)
