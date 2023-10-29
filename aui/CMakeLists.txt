client_module(aui)
client_module_sources(aui PRIVATE models)
client_module_link_libraries(aui PUBLIC scada_base)

# QT

target_compile_definitions(aui_qt PUBLIC UI_QT)

target_compile_features(aui_qt PUBLIC cxx_std_20)

find_package(Qt5 COMPONENTS Widgets REQUIRED)
target_link_libraries(aui_qt PUBLIC
  Qt5::Widgets
)

if(WIN32)
  find_package(Qt5 COMPONENTS AxContainer WinExtras REQUIRED)
  target_link_libraries(aui_qt PUBLIC
    Qt5::AxContainer
    Qt5::WinExtras
  )
endif()

target_compile_definitions(aui_qt PUBLIC UI_QT)

# WT

target_compile_definitions(aui_wt PUBLIC UI_WT)

target_compile_features(aui_wt PUBLIC cxx_std_20)

find_package(wt CONFIG REQUIRED)

target_link_libraries(aui_wt PUBLIC
  Wt::Wt
)

target_compile_definitions(aui_wt PUBLIC UI_WT)

# Subs.

add_subdirectory(test)