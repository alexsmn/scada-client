# The client's display surface: a QWidget over the `display` product's facade.
#
# ADR 0012 phase 3. It replaces client_vds_runtime, which dlopened
# tc_vds_runtime and blitted a BGRA buffer; phase 4 (task 747) deletes that
# module and the plugin ABI with it.
client_module(client_display_view CONFIG qt)

client_module_link_libraries(client_display_view
  PUBLIC
    client_base
    client_common
    display_view
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
  PRIVATE
    # Translate() for the operator-facing document and render failures.
    aui
)
