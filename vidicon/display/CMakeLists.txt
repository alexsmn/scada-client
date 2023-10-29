add_subdirectory(activex)
add_subdirectory(native)

client_module(client_vidicon_display CONFIG qt)

client_module_link_libraries(client_vidicon_display PUBLIC
  client_vidicon_display_activex
  client_vidicon_display_native
)
