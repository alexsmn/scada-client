client_component_module(client_component_accounts)

client_module_link_libraries(client_component_accounts PUBLIC
  node_service
  # ReadRoleMemberships / ApplyRoleMembershipChanges.
  client_user_access
)

if(TARGET client_component_accounts_qt_unittests)
  target_link_libraries(client_component_accounts_qt_unittests PRIVATE
    aui_test_qt
    scada_model
  )
endif()
