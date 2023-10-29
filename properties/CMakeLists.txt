add_subdirectory(transport)

client_module(client_properties)

client_module_link_libraries(client_properties PUBLIC
  aui
  node_service
)

client_module_link_libraries(client_properties PRIVATE
  client_properties_transport
  net
)

# UTs.

target_link_libraries(client_properties_qt_unittests PRIVATE
  address_space
  node_service_v1
)

target_link_libraries(client_properties_wt_unittests PRIVATE
  address_space
  node_service_v1
)