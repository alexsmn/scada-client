client_component_module(client_component_create)

client_module_link_libraries(client_component_create PRIVATE
  # The bulk-create wizard the "Multiple create" command now opens. The old
  # flat dialog (client_component_multi_create) is no longer reached from here
  # and is left in the tree until the wizard grows the Defaults step -- see the
  # backlog.
  client_bulk_create
  client_component_create_service_item
)

