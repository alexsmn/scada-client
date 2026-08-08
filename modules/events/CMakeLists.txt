client_module(client_events)

client_module_link_libraries(client_events PUBLIC
  scada_common
  # `event_view.cpp` uses `NetExecutorAdapter`, which transitively pulls in
  # `transport/executor.h`.
  transport
)

client_module_link_libraries(client_events PRIVATE
  client_controller
)

# UTs

if(TARGET client_events_qt_unittests)
  target_link_libraries(client_events_qt_unittests PRIVATE
    client_controller_test_qt
    # The severity-tile-strip test drives a real widget, so it needs a
    # QApplication (aui_test_qt).
    aui_test_qt
  )
endif()
