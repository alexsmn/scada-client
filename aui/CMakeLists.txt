client_module(aui)
client_module_sources(aui PRIVATE models)
client_module_link_libraries(aui PUBLIC
  scada_base
  scada_core
  transport
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

  target_compile_definitions(aui_qt PUBLIC UI_QT)

  # C++20 module facade (Qt flavor only; the wt flavor stays header-based).
  scada_add_module_facade(aui_qt FILES scada_client_aui.cppm
    IMPORTS scada_base_module scada_core_module)
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

  target_compile_definitions(aui_wt PUBLIC UI_WT)
endif()

# Subs.

add_subdirectory(test)
