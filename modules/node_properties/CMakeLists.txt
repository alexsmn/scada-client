client_component_module(client_component_node_properties)

if(TARGET client_component_node_properties_qt_unittests)
  target_link_libraries(client_component_node_properties_qt_unittests PRIVATE
    address_space
    client_properties_qt
    node_service_v3
  )
endif()

if(TARGET client_component_node_properties_qt)
  # client_properties_qt: NodePropertyModel calls PropertyService. Only this
  # module's unit test linked it, so a binary linking the module without the
  # test's list -- client_main_window_qt_unittests -- failed under GNU ld
  # (scada-client run 36301215089); macOS's linker resolved it anyway.
  target_link_libraries(client_component_node_properties_qt PRIVATE
    client_parameter_form_qt
    client_properties_qt
    client_ui_common_qt
  )
endif()
