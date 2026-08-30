client_module(client_configuration_objects)

client_module_link_libraries(client_configuration_objects PUBLIC
  client_controller
)

client_module_link_libraries(client_configuration_objects PRIVATE
  address_space
  client_configuration_tree
  client_services
  timed_data
)

# object_tree_view_unittest.cpp drives the real view, so it needs the shared
# ControllerContext fixture (client_controller_test_qt) and the Tree widget the
# marks live on (aui_qt).
if(TARGET client_configuration_objects_qt_unittests)
  target_link_libraries(client_configuration_objects_qt_unittests PRIVATE
    aui_qt
    client_controller_test_qt
  )
endif()
