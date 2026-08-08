client_component_module(client_component_watch)

if(TARGET client_component_watch_qt_unittests)
  target_link_libraries(client_component_watch_qt_unittests PRIVATE
    node_service
  )
endif()

if(TARGET client_component_watch_qt)
  target_link_libraries(client_component_watch_qt PRIVATE
    client_ui_common_qt
    # BuildDeviceAddressMap, for the decode pane's mapped-node rows. The query
    # browses a device's transmission items and is not really a properties
    # concern; it lives there because the device parameter form was its first
    # consumer.
    client_component_node_properties_qt
  )
endif()
