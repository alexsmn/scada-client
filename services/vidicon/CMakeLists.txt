scada_module(client_vidicon_module)

target_include_directories(client_vidicon_module PUBLIC "../..")

target_link_libraries(client_vidicon_module PUBLIC
  timed_data
)

target_sources(client_vidicon_module PRIVATE "teleclient_i.c")

# target_sources(client_vidicon_module PRIVATE "TeleClient.idl")
# build_idls(TARGET client_vidicon_module INCLUDE ${CMAKE_CURRENT_SOURCE_DIR})