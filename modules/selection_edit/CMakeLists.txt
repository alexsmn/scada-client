client_component_module(client_component_selection_edit)

if(TARGET client_component_selection_edit_qt)
  target_link_libraries(client_component_selection_edit_qt PRIVATE
    client_clipboard
    client_ui_common_qt
  )
endif()
