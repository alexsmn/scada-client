client_module(client_controller)

client_module_sources(client_controller PUBLIC "models")

client_module_link_libraries(client_controller PUBLIC
  aui
  client_base
  node_service
  # TODO: Remove the dependency from `common/aliases.h` and `scada_common`.
  scada_common
)

add_subdirectory(test)