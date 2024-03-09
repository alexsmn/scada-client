client_module(client_main_window)

client_module_sources(client_main_window PRIVATE status)

client_module_include_directories(client_main_window PUBLIC "..")

client_module_link_libraries(client_main_window PUBLIC aui)

client_module_link_libraries(client_main_window PRIVATE
  client_controller
  client_print_service
  net
)

# Qt.

# TODO: Remove dependency on `client_qt_lib`.
target_link_libraries(client_main_window_qt PRIVATE client_qt_lib)

client_qt_translations(client_main_window_qt "qt/main_window_ru.ts")

# Wt.

# TODO: Remove dependency on `client_wt_lib`.
target_link_libraries(client_main_window_wt PRIVATE client_wt_lib)

# UTs.

target_link_libraries(client_main_window_qt_unittests PRIVATE
  aui_test_qt
  # TODO: Remove dependency on `client_qt_lib`.
  client_qt_lib
)

target_link_libraries(client_main_window_wt_unittests PRIVATE
  aui_test_wt
  # TODO: Remove dependency on `client_wt_lib`.
  client_wt_lib
)
