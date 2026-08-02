client_module(client_vidicon_display_native CONFIG qt)

client_module_link_libraries(client_vidicon_display_native PUBLIC
  client_controller
)

client_module_link_libraries(client_vidicon_display_native PRIVATE
  client_display_frame
  client_filesystem
  client_vds_runtime
)
