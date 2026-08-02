client_module(client_favorites)

client_module_link_libraries(client_favorites PRIVATE
  client_controller
  # `favourites_view.cpp` uses `NetExecutorAdapter` for executor-pinned prompt
  # handling.
  transport
)

if(TARGET client_favorites_qt_unittests)
  target_link_libraries(client_favorites_qt_unittests PRIVATE aui_qt)
endif()
if(TARGET client_favorites_wt_unittests)
  target_link_libraries(client_favorites_wt_unittests PRIVATE aui_wt)
endif()
