add_subdirectory(transport)

client_module(client_properties)

client_module_link_libraries(client_properties PUBLIC
  aui
  node_service
)

client_module_link_libraries(client_properties PRIVATE
  client_properties_transport
  transport
)

# C++20 module facade.
if(TARGET client_properties_qt)
  scada_add_module_facade(client_properties_qt FILES scada_client_properties.cppm
    IMPORTS aui_qt_module node_service_module)
  if(SCADA_CXX_MODULES)
    add_subdirectory(module_test)
  endif()
endif()

# UTs.

if(TARGET client_properties_qt_unittests)
  target_link_libraries(client_properties_qt_unittests PRIVATE
    address_space
  )
endif()
