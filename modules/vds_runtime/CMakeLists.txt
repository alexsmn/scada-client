client_module(client_vds_runtime CONFIG qt)

# common/vds_runtime_api.h comes from scada_common, the target that owns it.
# It used to be reached as "${CMAKE_SOURCE_DIR}/common", which is the top-level
# source directory — /scada/common inside the superproject, and client/common in
# a standalone client build. That path exists too, so the build did not fail on a
# missing directory; it silently pointed at the wrong one and the header was not
# found.
client_module_include_directories(client_vds_runtime PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

client_module_link_libraries(client_vds_runtime
  PUBLIC
    client_base
    client_common
    scada_common
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
  PRIVATE
    # Translate() for the operator-facing document/render error messages.
    aui
)
