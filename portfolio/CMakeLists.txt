client_module(client_portfolio)

client_module_link_libraries(client_portfolio PUBLIC
  client_component_select_item
)

client_module_link_libraries(client_portfolio PRIVATE
  client_controller
)
