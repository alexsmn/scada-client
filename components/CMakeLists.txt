function(client_component_module module_name)
  cmake_parse_arguments(ARG "" "" "CONFIG" ${ARGN})

  if(ARG_CONFIG)
    client_module(${module_name} CONFIG ${ARG_CONFIG})
  else()
    client_module(${module_name})
  endif()

  client_module_sources(${module_name} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
  client_module_include_directories(${module_name} PUBLIC "..")

  client_module_link_libraries(${module_name} PUBLIC
    aui
  )

  client_module_link_libraries(${module_name} PRIVATE
    client_common
    client_controller
    client_events
    client_profile
    client_services
    transport
  )
endfunction()

foreach(component_name IN LISTS CLIENT_COMPONENTS)
  add_subdirectory(${component_name})
endforeach()
