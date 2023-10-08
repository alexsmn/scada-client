scada_module(client_vidicon_display INTERFACE)

target_link_libraries(client_vidicon_display INTERFACE
  client_controller_qt
)

# Expose component publicly to link it correctly.

target_sources(client_vidicon_display INTERFACE
  "vidicon_display_component.cpp"
  "vidicon_display_component.h"
)

add_subdirectory(qt)
add_subdirectory(tester)
