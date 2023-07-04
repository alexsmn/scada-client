client_module(client_controls)

client_module_link(client_controls PUBLIC scada_base)

# QT

target_compile_definitions(client_controls_qt PUBLIC UI_QT)

find_package(Qt5 COMPONENTS Widgets REQUIRED)

target_link_libraries(client_controls_qt PUBLIC
  Qt5::Widgets
)

if(WIN32)
  find_package(Qt5 COMPONENTS AxContainer WinExtras REQUIRED)
  target_link_libraries(client_controls_qt PUBLIC
    Qt5::AxContainer
    Qt5::WinExtras
  )
endif()

# WT

target_compile_definitions(client_controls_wt PUBLIC UI_WT)

find_package(wt CONFIG REQUIRED)

target_link_libraries(client_controls_wt PUBLIC
  Wt::Wt
)
