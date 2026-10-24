client_component_module(client_component_sheet)

# The model test reaches through sheet_cell.h into `timed_data`, which the
# module itself links privately.
if(TARGET client_component_sheet_qt_unittests)
  target_link_libraries(client_component_sheet_qt_unittests PRIVATE
    scada_common
    # The controller test builds a ControllerEnvironment, which holds a
    # FileCache.
    client_filesystem_qt
  )
endif()
