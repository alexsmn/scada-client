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

add_subdirectory(configuration)
add_subdirectory(export)
add_subdirectory(events)
add_subdirectory(favorites)
add_subdirectory(filesystem)
add_subdirectory(graph)
add_subdirectory(print)

if(BUILD_MODUS)
  add_subdirectory(modus)
endif()

# vidicon depends on scada_common_opc and scada_common_vidicon.
if(TARGET scada_common_opc AND TARGET scada_common_vidicon)
  add_subdirectory(vidicon)
endif()

foreach(component_name IN LISTS CLIENT_COMPONENTS)
  add_subdirectory(${component_name})
endforeach()

add_subdirectory(portfolio)
