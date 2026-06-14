client_component_module(client_component_transmission)

if(TARGET client_component_transmission_qt)
  target_link_libraries(client_component_transmission_qt PRIVATE
    client_ui_common_qt
  )
endif()
if(TARGET client_component_transmission_wt)
  target_link_libraries(client_component_transmission_wt PRIVATE
    client_ui_common_wt
  )
endif()
