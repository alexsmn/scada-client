client_module(client_profile)

client_module_link_libraries(client_profile PUBLIC aui)

client_module_link_libraries(client_profile PRIVATE
  client_base
  scada_common_events
)

target_link_libraries(client_profile_qt PRIVATE
  # For `FindWindowInfoByName`.
  client_controller_qt
)

target_link_libraries(client_profile_wt PRIVATE
  # For `FindWindowInfoByName`.
  client_controller_wt
)

# UTs.

# TODO: This should work:
# client_module_link_libraries(client_profile_unittests PRIVATE
#   client_controller
# )

target_link_libraries(client_profile_qt_unittests PRIVATE client_controller_qt)
target_link_libraries(client_profile_wt_unittests PRIVATE client_controller_wt)
