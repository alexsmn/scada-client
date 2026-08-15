client_module(client_favorites)

client_module_link_libraries(client_favorites PRIVATE
  client_controller
  # `favourites_view.cpp` uses `NetExecutorAdapter` for executor-pinned prompt
  # handling.
  transport
)

if(TARGET client_favorites_qt_unittests)
  # The add-to-favourites regression test needs a QApplication (aui_test_qt).
  target_link_libraries(client_favorites_qt_unittests PRIVATE
    aui_qt
    aui_test_qt
    client_controller_qt
  )
endif()
