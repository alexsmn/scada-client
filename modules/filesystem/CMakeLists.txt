client_module(client_filesystem)

client_module_link_libraries(client_filesystem PUBLIC
  scada_common
  # `file_manager_impl.cpp` uses `NetExecutorAdapter`, which transitively pulls
  # in `transport/executor.h`.
  transport
)

client_module_link_libraries(client_filesystem PRIVATE
  client_controller
)

# `file_cache_unittest.cpp` is auto-discovered by `client_module()` and
# picked up by the per-module `client_filesystem_qt_unittests`
# executable; it does not need to be added to the monolithic
# `client_qt_unittests` target as well.

# `filesystem_commands_unittest.cpp` pulls in `aui/dialog_service.h`,
# which relies on the `UI_QT` compile flag published by the `aui_qt`
# target. Link it so the flag and forward-declared `UiView` type are
# visible.
if(TARGET client_filesystem_qt_unittests)
  target_link_libraries(client_filesystem_qt_unittests PRIVATE aui_qt)
endif()
