client_component_module(client_component_node_table)

# The reshell Users table (UI_QT + reshell theme) renders the themed
# UsersGridPanel from the user_access module (backlog 5.1).
if(TARGET client_component_node_table_qt)
  target_link_libraries(client_component_node_table_qt PRIVATE
    client_user_access_qt)
endif()

# node_table_model_unittest drives the model over a real v3 node service +
# PropertyService against a code-defined test address space.
if(TARGET client_component_node_table_qt_unittests)
  target_link_libraries(client_component_node_table_qt_unittests PRIVATE
    address_space
    client_properties_qt
    node_service_v3
  )
endif()
