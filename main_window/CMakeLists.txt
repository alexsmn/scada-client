client_module(client_main_window)

client_module_sources(client_main_window PRIVATE status)

client_module_include_directories(client_main_window PUBLIC "..")

client_module_link_libraries(client_main_window PUBLIC aui)

client_module_link_libraries(client_main_window PRIVATE
  client_controller
  # TODO: Remove. Use dependecy injection.
  client_export_csv
  client_favorites
  client_filesystem
  client_portfolio
  client_print_service
  transport
)

# Qt.

# TODO: Remove dependency on `client_qt_core`.
target_link_libraries(client_main_window_qt PRIVATE client_qt_core winmm)

client_qt_translations(client_main_window_qt "qt/main_window_ru.ts")

# Wt.

# TODO: Remove dependency on `client_wt_lib`.
target_link_libraries(client_main_window_wt PRIVATE client_wt_lib)

# UTs.

target_link_libraries(client_main_window_qt_unittests PRIVATE
  aui_test_qt
  client_controller_test_qt
  client_core
  client_properties_qt
)

target_link_libraries(client_main_window_wt_unittests PRIVATE
  aui_test_wt
  client_controller_test_wt
  client_core
  client_properties_wt
)
