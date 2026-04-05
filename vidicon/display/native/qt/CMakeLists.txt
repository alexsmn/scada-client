scada_module(client_vidicon_display_qt)

target_link_libraries(client_vidicon_display_qt PRIVATE
  client_filesystem
  client_vidicon_display
  client_vidicon_module
)

# UTs.

target_link_libraries(client_vidicon_display_qt_unittests PRIVATE
  client_vidicon_module
)
