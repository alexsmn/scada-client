client_component_module(client_component_limits)

if(TARGET client_component_limits_qt_unittests)
  # The write-path regression test builds its analog item in a real node
  # service (FakeNodeService), which the module itself does not need.
  target_link_libraries(client_component_limits_qt_unittests PRIVATE
    address_space
    node_service
  )
endif()
