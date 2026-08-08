client_module(client_profile)

client_module_link_libraries(client_profile PUBLIC aui)

client_module_link_libraries(client_profile PRIVATE
  client_base
  scada_common_events
)

# C++20 module facade (Qt flavor only; the wt flavor stays header-based).
if(TARGET client_profile_qt)
  scada_add_module_facade(client_profile_qt FILES scada_client_profile.cppm
    IMPORTS aui_qt_module)
  if(SCADA_CXX_MODULES)
    add_subdirectory(module_test)
  endif()
endif()

# UTs.

# TODO: This should work:
# client_module_link_libraries(client_profile_unittests PRIVATE
#   client_controller
# )

if(TARGET client_profile_qt_unittests)
  target_link_libraries(client_profile_qt_unittests PRIVATE client_controller_qt)
endif()
if(TARGET client_profile_wt_unittests)
  target_link_libraries(client_profile_wt_unittests PRIVATE client_controller_wt)
endif()
