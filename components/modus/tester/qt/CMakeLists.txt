file(GLOB MODUS_TESTER_SOURCES CONFIGURE_DEPENDS
  "*.cpp"
  "*.h"
)

add_executable(modus_tester WIN32 ${MODUS_TESTER_SOURCES})

target_link_libraries(modus_tester PUBLIC
  display_tester
)
