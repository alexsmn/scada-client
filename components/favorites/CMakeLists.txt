client_module(client_favorites)

client_module_link_libraries(client_favorites PRIVATE
  client_controller
  # `favourites_view.cpp` uses `NetExecutorAdapter` / `AwaitPromise` for
  # executor-pinned prompt handling.
  transport
)

target_link_libraries(client_favorites_qt_unittests PRIVATE aui_qt)
target_link_libraries(client_favorites_wt_unittests PRIVATE aui_wt)
