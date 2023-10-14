scada_module(client_vidicon_teleclient)

target_include_directories(client_vidicon_teleclient PUBLIC "../..")

target_link_libraries(client_vidicon_teleclient PRIVATE
  scada_common_opc
  timed_data
)

find_package(MIDL REQUIRED)
add_idl(teleclient_idl "TeleClient.idl" INCLUDE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(client_vidicon_teleclient PRIVATE teleclient_idl)
target_include_directories(client_vidicon_teleclient PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

# UTs.

target_include_directories(client_vidicon_teleclient_unittests PRIVATE "..")

target_link_libraries(client_vidicon_teleclient_unittests PRIVATE
  timed_data
  OPC::opc
)
