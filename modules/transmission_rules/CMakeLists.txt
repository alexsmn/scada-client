client_module(client_transmission_rules CONFIG qt)

# Export the modules/ root so this module and its consumers can include
# "transmission_rules/qt/transmission_rule_inspector.h".
client_module_include_directories(client_transmission_rules PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

client_module_link_libraries(client_transmission_rules
  PUBLIC
    Qt6::Widgets
    # NodeRef appears in the public inspector header.
    node_service
  PRIVATE
    aui
    # TransmissionItemType / data-item type ids.
    scada_model
)

# The unit tests drive the pure logic and the inspector widget directly, so the
# test binary needs aui + a QApplication (aui_test_qt).
if(TARGET client_transmission_rules_qt_unittests)
  target_link_libraries(client_transmission_rules_qt_unittests PRIVATE
    aui_qt
    aui_test_qt
  )
endif()
