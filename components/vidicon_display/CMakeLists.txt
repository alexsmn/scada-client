client_module(client_vidicon_display CONFIG qt)

client_module_link_libraries(client_vidicon_display PRIVATE
  client_controller
  client_filesystem
  client_vidicon_module
)

# Expose component publicly to link it correctly.

target_sources(client_vidicon_display_qt INTERFACE
  "vidicon_display_component.cpp"
  "vidicon_display_component.h"
)

add_subdirectory(tester)
