add_subdirectory(display)

client_module(client_vidicon CONFIG qt)

client_module_link_libraries(client_vidicon PRIVATE
  client_vidicon_display
)
