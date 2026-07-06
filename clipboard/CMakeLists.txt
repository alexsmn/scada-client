# A special wrapper that hides protocol inside.

scada_module(client_clipboard)

# No PCH in the modules pilot (see core docs: scan-deps cannot read it).
if(NOT SCADA_CXX_MODULES)
  target_precompile_headers(client_clipboard REUSE_FROM scada_base)
endif()

target_link_libraries(client_clipboard PUBLIC
  node_service
  # `clipboard_util.cpp` uses coroutine awaitables, which transitively pull in
  # `transport/executor.h`.
  transport
)

target_link_libraries(client_clipboard PRIVATE
  scada_remote_protocol_conversions
)

# Modules-pilot consumer: node_serialization.cpp imports scada.node_service
# (see SCADA_USE_NODE_SERVICE_MODULE there).
scada_module_import_pilot(client_clipboard
  IMPORTS node_service_module
  DEFINES SCADA_USE_NODE_SERVICE_MODULE)

scada_add_module_facade(client_clipboard FILES scada_client_clipboard.cppm
  IMPORTS node_service_module)
if(SCADA_CXX_MODULES)
  add_subdirectory(module_test)
endif()
