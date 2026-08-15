client_module(client_portfolio)

client_module_link_libraries(client_portfolio PUBLIC
  client_component_select_item
)

client_module_link_libraries(client_portfolio PRIVATE
  client_controller
)

if(TARGET client_portfolio_qt_unittests)
  target_link_libraries(client_portfolio_qt_unittests PRIVATE
    # The tree-model test builds its portfolios against a real node service.
    node_service
    scada_common
  )
endif()
