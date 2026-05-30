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
  )
endif()


if(TARGET client_events_wt_unittests)
  target_link_libraries(client_events_wt_unittests PRIVATE
    aui_test_wt
    client_controller_test_wt
  )
endif()
