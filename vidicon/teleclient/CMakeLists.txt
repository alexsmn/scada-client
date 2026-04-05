scada_module(client_vidicon_teleclient)

target_precompile_headers(client_vidicon_teleclient REUSE_FROM scada_base)

target_include_directories(client_vidicon_teleclient PUBLIC "../..")

target_link_libraries(client_vidicon_teleclient PRIVATE
  scada_common_opc
  scada_common_vidicon
  timed_data
)

# UTs.

target_include_directories(client_vidicon_teleclient_unittests PRIVATE "..")

target_link_libraries(client_vidicon_teleclient_unittests PRIVATE
  scada_common_opc
  scada_common_vidicon
  timed_data
)
