client_module(client_properties_transport)

client_module_link_libraries(client_properties_transport PUBLIC
  aui
  net
)

# Qt.

target_sources(client_properties_transport_qt PRIVATE
  "qt/transport_dialog.ui"
)

client_qt_translations(client_properties_transport_qt
  "qt/transport_dialog_ru.ts"
)