client_component_module(client_component_summary)

target_link_libraries(client_component_summary_qt PRIVATE
  client_ui_common_qt
  transport
)
target_link_libraries(client_component_summary_wt PRIVATE
  client_ui_common_wt
  transport
)

target_link_libraries(client_component_summary_qt_unittests PRIVATE
  scada_common
)
target_link_libraries(client_component_summary_wt_unittests PRIVATE
  scada_common
)
