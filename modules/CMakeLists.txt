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
# The display surface and everything that draws through it. `modus` and
# `vidicon` are further down for ordering reasons but belong to this group:
# both link client_display_view, so neither can build without `display`.
# Their absence is already a supported state — app/qt and main_window gate on
# `if(TARGET client_modus_qt)` — which is what makes this option cheap.
if(CLIENT_WITH_DISPLAY)
  add_subdirectory(display_view)
  add_subdirectory(display_frame)
endif()
add_subdirectory(inspector)
add_subdirectory(device_diagnostics)
add_subdirectory(parameter_form)
add_subdirectory(administration)
add_subdirectory(settings)
add_subdirectory(user_access)
add_subdirectory(transmission_rules)
add_subdirectory(bulk_create)

# See the note above display_view: these two draw through it.
if(CLIENT_WITH_DISPLAY)
  add_subdirectory(modus)
  add_subdirectory(vidicon)
endif()

foreach(component_name IN LISTS CLIENT_COMPONENTS)
  add_subdirectory(${component_name})
endforeach()

add_subdirectory(portfolio)
