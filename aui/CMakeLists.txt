client_module(aui)
client_module_sources(aui PRIVATE models)

# Keep this link set minimal and honest: aui is the platform-agnostic UI
# layer, slated for extraction into its own repository. Its project
# dependencies are scada_base (core/base utilities) and the standalone view
# component libraries (graph_qt, view_manager_qt — declared below for the Qt
# flavor); everything else it needs is Qt (aui_qt) or Wt (aui_wt). Do not add
# links to client-repo modules here — invert the dependency instead (see
# docs/aui-extraction.md).
client_module_link_libraries(aui PUBLIC
  scada_base
)

# QT

if(TARGET aui_qt)
  target_compile_definitions(aui_qt PUBLIC UI_QT)

  target_compile_features(aui_qt PUBLIC cxx_std_23)

  find_package(Qt6 COMPONENTS Widgets REQUIRED)
  target_link_libraries(aui_qt PUBLIC
    Qt6::Widgets
  )

  if(WIN32)
    find_package(Qt6 COMPONENTS AxContainer REQUIRED)
    target_link_libraries(aui_qt PUBLIC
      Qt6::AxContainer
    )
  endif()

  # Abstract-UI seams over the standalone view component libraries:
  # aui/qt/graph.h wraps graph_qt, aui/view_manager.h wraps
  # view_manager_qt. Declared here so aui owns the include paths its own
  # headers use instead of borrowing them from consumer targets
  # (client_graph / main_window). Both find modules are resolved through the
  # preset-seeded CMAKE_MODULE_PATH.
  find_package(GraphQt REQUIRED)
  find_package(ViewManagerQt REQUIRED)
  target_link_libraries(aui_qt PUBLIC
    graph_qt
    view_manager_qt
  )

  # C++20 module facade (Qt flavor only; the wt flavor stays header-based).
  scada_add_module_facade(aui_qt FILES scada_client_aui.cppm
    IMPORTS scada_base_module)
  if(SCADA_CXX_MODULES)
    add_subdirectory(module_test)
  endif()
endif()

# WT

if(TARGET aui_wt)
  target_compile_definitions(aui_wt PUBLIC UI_WT)

  target_compile_features(aui_wt PUBLIC cxx_std_23)

  find_package(wt CONFIG REQUIRED)

  target_link_libraries(aui_wt PUBLIC
    Wt::Wt
  )
endif()

# Subs.

add_subdirectory(test)
