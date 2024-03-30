client_module(client_events)

client_module_link_libraries(client_events PRIVATE
  client_controller
)

# UTs

target_link_libraries(client_events_qt PRIVATE
  client_controller_test_qt
)


target_link_libraries(client_events_wt PRIVATE
  aui_test_wt
  client_controller_test_wt
)
