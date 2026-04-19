client_module(client_services)

client_module_link_libraries(client_services PUBLIC
  aui
  scada_common
  # `task_manager_impl.cpp` uses `NetExecutorAdapter` / `scada::CoroutineService`
  # adapters, which transitively pull in `transport/executor.h`.
  transport
)

# UTs.

target_link_libraries(client_services_qt_unittests PRIVATE
  address_space
  client_core
  client_events_qt
  client_profile_qt
  node_service
)
if(MSVC)
  target_link_options(client_services_qt_unittests PRIVATE
    "$<$<CONFIG:RelWithDebInfo>:/DEBUG:NONE>"
    "$<$<CONFIG:RelWithDebInfo>:/INCREMENTAL:NO>"
    "$<$<CONFIG:RelWithDebInfo>:/PDB:NONE>"
  )
endif()

target_link_libraries(client_services_wt_unittests PRIVATE
  address_space
  client_core
  client_events_wt
  client_profile_wt
  node_service
)
