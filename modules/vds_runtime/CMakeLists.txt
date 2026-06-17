client_module(client_vds_runtime CONFIG qt)

client_module_include_directories(client_vds_runtime PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/.."
  "${CMAKE_SOURCE_DIR}/common"
)

client_module_link_libraries(client_vds_runtime PUBLIC
  client_base
  client_common
  Qt6::Core
  Qt6::Gui
  Qt6::Widgets
)
