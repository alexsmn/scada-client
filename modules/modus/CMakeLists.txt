client_module(client_modus CONFIG qt)

if(TARGET client_modus_qt)
  get_target_property(_client_modus_qt_sources client_modus_qt SOURCES)
  list(FILTER _client_modus_qt_sources EXCLUDE REGEX
    "qt/modus_view(2)?\\.(cpp|h)$")
  set_target_properties(client_modus_qt PROPERTIES
    SOURCES "${_client_modus_qt_sources}")
endif()

client_module_link_libraries(client_modus PRIVATE
  client_controller
  client_vds_runtime
)

if(TARGET client_modus_qt_unittests)
  target_link_libraries(client_modus_qt_unittests PRIVATE
    aui_qt
    client_filesystem_qt
  )
endif()
