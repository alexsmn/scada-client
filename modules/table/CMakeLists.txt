client_component_module(client_component_table)

if(TARGET client_component_table_qt)
  target_link_libraries(client_component_table_qt PRIVATE
    client_ui_common_qt
  )
endif()
if(TARGET client_component_table_wt)
  target_link_libraries(client_component_table_wt PRIVATE
    client_ui_common_wt
  )
endif()

if(TARGET client_component_table_qt_unittests)
  target_link_libraries(client_component_table_qt_unittests PRIVATE
    scada_common
  )
endif()
if(TARGET client_component_table_wt_unittests)
  target_link_libraries(client_component_table_wt_unittests PRIVATE
    scada_common
  )
endif()
