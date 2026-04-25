# A special wrapper that hides protocol inside.

scada_module(client_clipboard)

target_precompile_headers(client_clipboard REUSE_FROM scada_base)

target_link_libraries(client_clipboard PUBLIC
  node_service
  # `clipboard_util.cpp` uses coroutine awaitables, which transitively pull in
  # `transport/executor.h`.
  transport
)

target_link_libraries(client_clipboard PRIVATE
  scada_remote_protocol_conversions
)
