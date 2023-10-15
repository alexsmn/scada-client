add_subdirectory(activex)
add_subdirectory(native)

client_module(client_vidicon_display CONFIG qt)

client_module_link_libraries(client_vidicon_display PRIVATE
  client_vidicon_display_activex
  client_vidicon_display_native
)

# Expose component publicly to link it correctly.

target_sources(client_vidicon_display_qt INTERFACE
  "vidicon_display_component.cpp"
  "vidicon_display_component.h"
)
