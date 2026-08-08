client_component_module(client_component_summary)

if(TARGET client_component_summary_qt)
  target_link_libraries(client_component_summary_qt PRIVATE
    client_ui_common_qt
    transport
  )
endif()
if(TARGET client_component_summary_wt)
  target_link_libraries(client_component_summary_wt PRIVATE
    client_ui_common_wt
    transport
  )
endif()

if(TARGET client_component_summary_qt_unittests)
  target_link_libraries(client_component_summary_qt_unittests PRIVATE
    scada_common
  )
endif()
if(TARGET client_component_summary_wt_unittests)
  target_link_libraries(client_component_summary_wt_unittests PRIVATE
    scada_common
  )
endif()
