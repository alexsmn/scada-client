scada_module(display_tester)

target_link_libraries(display_tester PUBLIC
  aui_qt
  timed_data
)

target_link_libraries(display_tester PRIVATE
  client_controller_qt
)
