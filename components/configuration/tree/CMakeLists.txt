client_module(client_configuration_tree)

client_module_link_libraries(client_configuration_tree PUBLIC
  aui
  node_service
  client_ui_dragdrop
  # `configuration_tree_drop_handler.cpp` uses `NetExecutorAdapter` /
  # `AwaitPromise`, which transitively pull in `transport/executor.h`.
  transport
)
