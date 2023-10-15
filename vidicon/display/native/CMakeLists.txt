client_module(client_vidicon_display_native CONFIG qt)

client_module_link_libraries(client_vidicon_display_native PUBLIC
  client_controller
)

client_module_link_libraries(client_vidicon_display_native PRIVATE
  client_filesystem
  client_vidicon_teleclient
  scada_common_vidicon
)
