client_module(client_display_frame CONFIG qt)

# Export the modules/ root so this module and its consumers can include
# "display_frame/qt/display_frame.h".
client_module_include_directories(client_display_frame PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

client_module_link_libraries(client_display_frame
  PUBLIC
    Qt6::Widgets
    # Public: DisplayFrameContext / EventObserver appear in the public header.
    scada_common_events
  PRIVATE
    aui
    # `events/event_severity.h` — the strip bands severities through the shared
    # SeverityLevelForEvent rather than re-implementing the thresholds (739's
    # sibling, backlog 624).
    client_events
    client_display_view
    node_service
    timed_data
)

# The unit tests exercise the single-line style mapping directly against the
# aui theme tokens, so the test binary needs aui on its own compile/link line
# (aui is a PRIVATE dependency of the library).
if(TARGET client_display_frame_qt_unittests)
  # aui_qt: theme tokens for the style mapping; aui_test_qt: AppEnvironment
  # (a QApplication) for the widget-level strip tests.
  target_link_libraries(client_display_frame_qt_unittests PRIVATE
    aui_qt
    aui_test_qt
  )
endif()
