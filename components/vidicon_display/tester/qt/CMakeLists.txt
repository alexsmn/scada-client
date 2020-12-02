file(GLOB VIDICON_DISPLAY_TESTER_SOURCES CONFIGURE_DEPENDS
  "*.cpp"
  "*.h"
)

add_executable(vidicon_display_tester WIN32 ${VIDICON_DISPLAY_TESTER_SOURCES})

find_package(Qt5 COMPONENTS Core Widgets REQUIRED)
target_link_libraries(vidicon_display_tester PUBLIC
  client_qt_lib
)
