client_component_module(client_component_write)

if(TARGET client_component_write_qt_unittests)
  target_link_libraries(client_component_write_qt_unittests PRIVATE
    node_service
  )
endif()
if(TARGET client_component_write_wt_unittests)
  target_link_libraries(client_component_write_wt_unittests PRIVATE
    node_service
  )
endif()
