add_subdirectory(teleclient)
add_subdirectory(display)
add_subdirectory(display_tester)

client_module(client_vidicon CONFIG qt)

client_module_link_libraries(client_vidicon PRIVATE
  client_vidicon_display
  client_vidicon_teleclient
)
