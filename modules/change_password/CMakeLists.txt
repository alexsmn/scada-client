client_component_module(client_component_change_password)

client_module_link_libraries(client_component_change_password PUBLIC
  node_service
)

if(TARGET client_component_change_password_qt_unittests)
  target_link_libraries(client_component_change_password_qt_unittests PRIVATE
    client_core
  )
endif()

if(TARGET client_component_change_password_wt_unittests)
  target_link_libraries(client_component_change_password_wt_unittests PRIVATE
    client_core
  )
endif()
