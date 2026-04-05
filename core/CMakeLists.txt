scada_module(client_core)

target_precompile_headers(client_core REUSE_FROM scada_base)

target_link_libraries(client_core PUBLIC scada_base)
