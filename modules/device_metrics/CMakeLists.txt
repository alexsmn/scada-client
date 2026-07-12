client_component_module(client_component_device_metrics)

if(TARGET client_component_device_metrics_qt)
  target_link_libraries(client_component_device_metrics_qt PRIVATE
    client_component_sheet_qt
    client_ui_common_qt
  )
endif()
if(TARGET client_component_device_metrics_wt)
  target_link_libraries(client_component_device_metrics_wt PRIVATE
    client_component_sheet_wt
    client_ui_common_wt
  )
endif()

if(TARGET client_component_device_metrics_qt_unittests)
  target_link_libraries(client_component_device_metrics_qt_unittests PRIVATE
    address_space
    node_service
  )
endif()
if(TARGET client_component_device_metrics_wt_unittests)
  target_link_libraries(client_component_device_metrics_wt_unittests PRIVATE
    address_space
    node_service
  )
endif()
