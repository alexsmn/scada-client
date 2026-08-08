client_component_module(client_component_node_properties)

if(TARGET client_component_node_properties_qt_unittests)
  target_link_libraries(client_component_node_properties_qt_unittests PRIVATE
    address_space
    client_properties_qt
    node_service_v3
  )
endif()

if(TARGET client_component_node_properties_qt)
  target_link_libraries(client_component_node_properties_qt PRIVATE
    client_parameter_form_qt
    client_ui_common_qt
  )
endif()
