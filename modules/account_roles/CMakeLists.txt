client_component_module(client_component_account_roles)

client_module_link_libraries(client_component_account_roles PUBLIC
  node_service
  # ReadRoleMemberships / ApplyRoleMembershipChanges.
  client_user_access
)

if(TARGET client_component_account_roles_qt_unittests)
  target_link_libraries(client_component_account_roles_qt_unittests PRIVATE
    aui_test_qt
    scada_model
  )
endif()
