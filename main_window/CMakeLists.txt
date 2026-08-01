client_module(client_main_window)

# The client build version, surfaced in the status strip's endpoint cell.
foreach(cfg IN LISTS CLIENT_UI_CONFIGS)
  if(TARGET client_main_window_${cfg})
    target_compile_definitions(client_main_window_${cfg} PRIVATE
      CLIENT_BUILD_VERSION="${PROJECT_VERSION}")
  endif()
endforeach()

add_subdirectory(main_menu)
add_subdirectory(opened_view)
add_subdirectory(pages)
add_subdirectory(status_bar)

client_module_include_directories(client_main_window PUBLIC "..")

client_module_link_libraries(client_main_window PUBLIC aui)
client_module_link_libraries(client_main_window PUBLIC
  ${CLIENT_COMPONENT_MODULES}
)
client_module_link_libraries(client_main_window PUBLIC
  client_graph
)

client_module_link_libraries(client_main_window PRIVATE
  client_controller
  # TODO: Remove. Use dependecy injection.
  client_export_csv
  client_export_excel
  client_favorites
  client_filesystem
  client_portfolio
  client_print_service
  transport
)

# Qt.

list(APPEND CMAKE_MODULE_PATH
  "${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/view_manager_qt"
)
if(TARGET client_main_window_qt)
  find_package(ViewManagerQt REQUIRED)

  target_link_libraries(client_main_window_qt PUBLIC
    view_manager_qt
  )

  target_link_libraries(client_main_window_qt PRIVATE
    client_device_diagnostics_qt
    client_events_qt
    client_inspector_qt
    client_user_access_qt
    client_transmission_rules_qt
    client_ui_common_qt
    client_ui_dragdrop_qt
    client_ui_progress_qt
  )
  if(WIN32)
    target_link_libraries(client_main_window_qt PRIVATE winmm)
  endif()
  if(TARGET client_modus_qt)
    target_compile_definitions(client_main_window_qt PUBLIC CLIENT_HAS_MODUS=1)
  else()
    target_compile_definitions(client_main_window_qt PUBLIC CLIENT_HAS_MODUS=0)
  endif()

  client_qt_translations(client_main_window_qt "main_window_ru_qt.ts")
endif()

# Wt.

# TODO: Remove dependency on `client_wt_lib`.
if(TARGET client_main_window_wt)
  target_link_libraries(client_main_window_wt PRIVATE client_wt_lib)
endif()

# UTs.

if(TARGET client_main_window_qt_unittests)
  target_link_libraries(client_main_window_qt_unittests PRIVATE
    aui_test_qt
    client_controller_test_qt
    client_core
    client_properties_qt
  )
  # The activity rail renders its section glyphs from res/icons/*.svg
  # (docs/client/ux/iconography.md §5.3), so the resource has to be in this
  # binary or every rail icon is null. A separate exe from the app, so no
  # duplicate qInitResources_client.
  set_target_properties(client_main_window_qt_unittests PROPERTIES AUTORCC ON)
  target_sources(client_main_window_qt_unittests PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../res/client.qrc")
endif()

if(TARGET client_main_window_wt_unittests)
  target_link_libraries(client_main_window_wt_unittests PRIVATE
    aui_test_wt
    client_controller_test_wt
    client_core
    client_properties_wt
  )
endif()
