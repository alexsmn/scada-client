client_component_module(client_component_timed_data)

if(TARGET client_component_timed_data_qt_unittests)
  target_link_libraries(client_component_timed_data_qt_unittests PRIVATE
    timed_data
  )
endif()
