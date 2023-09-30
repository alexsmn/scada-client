scada_module(client_controller INTERFACE)

target_link_libraries(client_controller INTERFACE node_service)

target_compile_features(client_controller INTERFACE cxx_std_17)

# QT

add_library(client_controller_qt)

target_link_libraries(client_controller_qt PUBLIC
  aui_qt
  client_controller
)

# WT

add_library(client_controller_wt)

target_link_libraries(client_controller_wt PUBLIC
  aui_wt
  client_controller
)
