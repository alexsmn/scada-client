client_component_module(client_component_selection_edit)

client_module_link_libraries(client_component_selection_edit PUBLIC
  # IsUndeletableAccount: Delete is not offered on a NoDelete account.
  client_user_access
)

if(TARGET client_component_selection_edit_qt)
  target_link_libraries(client_component_selection_edit_qt PRIVATE
    client_clipboard
    client_ui_common_qt
  )
endif()
