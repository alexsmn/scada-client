client_module(client_controller)

client_module_link_libraries(client_controller PUBLIC
  aui
  client_base
  # For `WindowDefinition`.
  client_profile
  node_service
  timed_data
)

add_subdirectory(test)