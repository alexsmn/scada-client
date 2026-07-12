client_module(client_display_frame CONFIG qt)

# Export the modules/ root so this module and its consumers can include
# "display_frame/qt/display_frame.h".
client_module_include_directories(client_display_frame PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

client_module_link_libraries(client_display_frame
  PUBLIC
    Qt6::Widgets
  PRIVATE
    aui
    client_vds_runtime
)
