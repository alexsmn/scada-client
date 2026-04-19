client_module(client_filesystem)

client_module_link_libraries(client_filesystem PUBLIC
  scada_common
  # `file_manager_impl.cpp` uses `NetExecutorAdapter` / `AwaitPromise`, which
  # transitively pull in `transport/executor.h`.
  transport
)

client_module_link_libraries(client_filesystem PRIVATE
  client_controller
)

# `file_cache_unittest.cpp` is auto-discovered by `client_module()` and
# picked up by the per-module `client_filesystem_{qt,wt}_unittests`
# executables; it does not need to be added to the monolithic
# `client_{qt,wt}_unittests` target as well.
