client_module(client_events)

client_module_link_libraries(client_events PUBLIC
  scada_common
  # `event_view.cpp` uses `NetExecutorAdapter` / `AwaitPromise`, which
  # transitively pull in `transport/executor.h`.
  transport
)

client_module_link_libraries(client_events PRIVATE
  client_controller
)

# UTs

target_link_libraries(client_events_qt_unittests PRIVATE
  client_controller_test_qt
)


target_link_libraries(client_events_wt_unittests PRIVATE
  aui_test_wt
  client_controller_test_wt
)
