client_module(client_vidicon_display_activex CONFIG qt)

client_module_link_libraries(client_vidicon_display_activex PRIVATE
  client_controller
  client_vidicon_teleclient
)
