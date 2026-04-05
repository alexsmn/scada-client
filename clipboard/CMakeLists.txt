# A special wrapper that hides protocol inside.

scada_module(client_clipboard)

target_link_libraries(client_clipboard PUBLIC
  node_service
)

target_link_libraries(client_clipboard PRIVATE
  scada_remote_protocol_conversions
)
