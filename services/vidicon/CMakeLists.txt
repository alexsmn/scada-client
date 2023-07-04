scada_module(client_vidicon_module)

target_include_directories(client_vidicon_module PUBLIC "../..")

# `TimedDataService` is provided from outside.
target_link_libraries(client_vidicon_module PUBLIC timed_data)

add_idl(teleclient_idl "TeleClient.idl" INCLUDE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(client_vidicon_module PRIVATE teleclient_idl)
# Include generated IDL privately.
target_include_directories(client_vidicon_module PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

# UTs.

# Include generated IDL privately. UTs use IDL interfaces as a client.
target_include_directories(client_vidicon_module_unittests PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
