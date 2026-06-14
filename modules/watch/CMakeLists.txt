client_component_module(client_component_watch)

if(TARGET client_component_watch_qt_unittests)
  target_link_libraries(client_component_watch_qt_unittests PRIVATE
    node_service
  )
endif()
if(TARGET client_component_watch_wt_unittests)
  target_link_libraries(client_component_watch_wt_unittests PRIVATE
    node_service
  )
endif()

if(TARGET client_component_watch_qt)
  target_link_libraries(client_component_watch_qt PRIVATE
    client_ui_common_qt
  )
endif()
if(TARGET client_component_watch_wt)
  target_link_libraries(client_component_watch_wt PRIVATE
    client_ui_common_wt
  )
endif()
