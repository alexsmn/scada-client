client_module(client_controller_test)

client_module_link_libraries(client_controller_test PUBLIC
  client_controller
)

target_link_libraries(client_controller_test_qt PUBLIC
  client_events_qt
  client_filesystem_qt
  client_portfolio_qt
  client_services_qt
)

target_link_libraries(client_controller_test_wt PUBLIC
  client_events_wt
  client_filesystem_wt
  client_portfolio_wt
  client_services_wt
)