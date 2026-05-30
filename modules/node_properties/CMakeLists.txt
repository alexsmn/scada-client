client_component_module(client_component_node_properties)

if(TARGET client_component_node_properties_qt_unittests)
  target_link_libraries(client_component_node_properties_qt_unittests PRIVATE
    address_space
    client_properties_qt
    node_service_v1
  )
endif()
if(TARGET client_component_node_properties_wt_unittests)
  target_link_libraries(client_component_node_properties_wt_unittests PRIVATE
    address_space
    client_properties_wt
    node_service_v1
  )
endif()
