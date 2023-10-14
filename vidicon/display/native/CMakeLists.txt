client_module(client_vidicon_display_native CONFIG qt)

client_module_link_libraries(client_vidicon_display_native PRIVATE
  client_controller
  client_filesystem
  client_vidicon_teleclient
)
