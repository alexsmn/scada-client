client_module(client_services)

client_module_link_libraries(client_services PUBLIC
  aui
  scada_common
  # `task_manager_impl.cpp` uses `NetExecutorAdapter` / `scada::CoroutineService`
  # adapters, which transitively pull in `transport/executor.h`.
  transport
)

if(APPLE)
  # Objective-C++ shim behind `AppNapSuppressor` (see app_nap_suppressor.h).
  # It gets its own target for two reasons: `client_services_*` reuses
  # scada_base's C++ precompiled header, which cannot be applied to an
  # Objective-C++ translation unit; and `scada_module()` only globs *.cpp/*.h,
  # so a .mm left in this directory would be silently dropped from the build.
  enable_language(OBJCXX)
  add_library(client_app_nap_mac STATIC app_nap_suppressor_mac.mm)
  target_include_directories(client_app_nap_mac
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/..")
  target_link_libraries(client_app_nap_mac PUBLIC "-framework Foundation")
  client_module_link_libraries(client_services PUBLIC client_app_nap_mac)
endif()

# C++20 module facade (Qt flavor only; the wt flavor stays header-based).
if(TARGET client_services_qt)
  scada_add_module_facade(client_services_qt FILES scada_client_services.cppm
    IMPORTS aui_qt_module scada_common_module)
  if(SCADA_CXX_MODULES)
    add_subdirectory(module_test)
  endif()
endif()

# UTs.

if(TARGET client_services_qt_unittests)
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
endif()

if(TARGET client_services_wt_unittests)
  target_link_libraries(client_services_wt_unittests PRIVATE
    address_space
    client_core
    client_events_wt
    client_profile_wt
    node_service
  )
endif()
