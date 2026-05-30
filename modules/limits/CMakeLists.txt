client_component_module(client_component_limits)

if(TARGET client_component_limits_wt_unittests)
  target_link_libraries(client_component_limits_wt_unittests PRIVATE
    node_service
  )
endif()
