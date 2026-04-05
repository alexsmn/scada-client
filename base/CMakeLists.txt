scada_module(client_base)

target_precompile_headers(client_base REUSE_FROM scada_base)

target_link_libraries(client_base PUBLIC scada_base)
