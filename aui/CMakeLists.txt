find_package(Qt5 COMPONENTS Widgets REQUIRED)

# Must be set befire `client_module`.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

client_module(aui)

client_module_link(aui PUBLIC scada_base)

# QT

target_compile_definitions(aui_qt PUBLIC UI_QT)

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

target_compile_definitions(aui_qt PUBLIC "UI_QT")

# WT

target_compile_definitions(aui_wt PUBLIC UI_WT)

find_package(wt CONFIG REQUIRED)

target_link_libraries(aui_wt PUBLIC
  Wt::Wt
)

target_compile_definitions(aui_wt PUBLIC "UI_WT")