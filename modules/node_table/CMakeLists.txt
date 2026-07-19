client_component_module(client_component_node_table)

# The reshell Users table (UI_QT + reshell theme) renders the themed
# UsersGridPanel from the user_access module (backlog 5.1).
if(TARGET client_component_node_table_qt)
  target_link_libraries(client_component_node_table_qt PRIVATE
    client_user_access_qt)
endif()
