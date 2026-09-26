client_module(client_configuration_devices)

client_module_link_libraries(client_configuration_devices PUBLIC
  client_controller
)

client_module_link_libraries(client_configuration_devices PRIVATE
  aui
  client_configuration_tree
  client_services
)

# TransmissionRuleSummary and FetchTransmissionRule, for the rule rows' label.
# Linked by concrete target name rather than through
# client_module_link_libraries: that helper resolves a name only if the target
# already exists, and modules/transmission_rules is added after this directory,
# so the name would be dropped without a word and the link would fail on the
# two symbols instead.
if(TARGET client_configuration_devices_qt)
  target_link_libraries(client_configuration_devices_qt PRIVATE
    client_transmission_rules_qt
  )
endif()

# hardware_tree_model_unittest.cpp builds the real model over the shared test
# address space, so it needs the node states (address_space) and the timed-data
# fake the model's device-state notifier takes.
if(TARGET client_configuration_devices_qt_unittests)
  target_link_libraries(client_configuration_devices_qt_unittests PRIVATE
    address_space
    timed_data
  )
endif()
