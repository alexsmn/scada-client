client_module(client_administration)

client_module_link_libraries(client_administration PRIVATE
  client_controller
  # RolesView is registered here rather than beside its own class, so this
  # module owns the dependency (see administration_module.cpp).
  client_user_access
)
