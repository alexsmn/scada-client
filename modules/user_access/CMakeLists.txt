client_module(client_user_access CONFIG qt)

# Export the modules/ root so this module and its consumers can include
# "user_access/qt/user_access_panel.h".
client_module_include_directories(client_user_access PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

client_module_link_libraries(client_user_access
  PUBLIC
    Qt6::Widgets
    # NodeRef appears in the public panel header.
    node_service
    # RolesView derives from Controller and registers itself.
    controller
  PRIVATE
    aui
    # UserType / RoleType / IdentityMappingRuleType ids.
    scada_model
)

# The unit tests drive the pure RBAC logic and the panel widget directly, so the
# test binary needs aui + a QApplication (aui_test_qt).
if(TARGET client_user_access_qt_unittests)
  target_link_libraries(client_user_access_qt_unittests PRIVATE
    aui_qt
    aui_test_qt
    # FakeNodeService drives ShowUser end-to-end; it needs the security ids and
    # NodeState, which the library itself only links privately.
    scada_model
    scada_common
  )
endif()
