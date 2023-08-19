file(GLOB MODUS_TESTER_SOURCES CONFIGURE_DEPENDS
  "*.cpp"
  "*.h"
)

add_executable(modus_tester WIN32 ${MODUS_TESTER_SOURCES})

find_package(Qt5 COMPONENTS Core Widgets REQUIRED)
target_link_libraries(modus_tester PUBLIC
  client_qt_lib
)
