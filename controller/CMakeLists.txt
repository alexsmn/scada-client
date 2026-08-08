client_module(client_controller)

client_module_link_libraries(client_controller PUBLIC
  aui
  client_base
  # For `WindowDefinition`.
  client_profile
  node_service
  timed_data
)

# C++20 module facade.
if(TARGET client_controller_qt)
  scada_add_module_facade(client_controller_qt FILES scada_client_controller.cppm
    IMPORTS
      aui_qt_module
      client_base_module
      client_profile_qt_module
      node_service_module
      timed_data_module)
  if(SCADA_CXX_MODULES)
    add_subdirectory(module_test)
  endif()
endif()

add_subdirectory(test)