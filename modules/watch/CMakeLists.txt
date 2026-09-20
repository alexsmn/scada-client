client_component_module(client_component_watch)

if(TARGET client_component_watch_qt_unittests)
  target_link_libraries(client_component_watch_qt_unittests PRIVATE
    client_events_qt
    node_service
  )
endif()

if(TARGET client_component_watch_qt)
  target_link_libraries(client_component_watch_qt PRIVATE
    client_ui_common_qt
    # `events/event_severity.h`: the device log bands its rows by severity, and
    # that banding is shared with the journal and the status strip so a
    # threshold change moves every severity surface at once.
    client_events_qt
    # BuildDeviceAddressMap, for the decode pane's mapped-node rows. The query
    # browses a device's transmission items and is not really a properties
    # concern; it lives there because the device parameter form was its first
    # consumer.
    client_component_node_properties_qt
  )
endif()
