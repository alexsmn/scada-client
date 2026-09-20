client_module(client_configuration_devices)

client_module_link_libraries(client_configuration_devices PUBLIC
  client_controller
)

client_module_link_libraries(client_configuration_devices PRIVATE
  aui
  client_configuration_tree
  client_services
)

# hardware_tree_model_unittest.cpp builds the real model over the shared test
# address space, so it needs the node states (address_space) and the timed-data
# fake the model's device-state notifier takes.
if(TARGET client_configuration_devices_qt_unittests)
  target_link_libraries(client_configuration_devices_qt_unittests PRIVATE
    address_space
    timed_data
  )
endif()
