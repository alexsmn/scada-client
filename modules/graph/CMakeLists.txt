client_module(client_graph CONFIG qt)

if(TARGET client_graph_qt)
  find_package(GraphQt REQUIRED)
endif()

client_module_link_libraries(client_graph PRIVATE
  # Reshell chart chrome reads the shared design tokens (theme_qt.h) and the
  # opt-in gate (severity_colors.h); both live in aui.
  aui
  client_controller
  client_filesystem
  graph
  timed_data
  # `metrix_data_source.cpp` uses coroutine awaitables for executor-pinned
  # history reads.
  transport
)

if(TARGET client_graph_qt_unittests)
  target_link_libraries(client_graph_qt_unittests PRIVATE
    aui_qt
    client_controller_test_qt
    graph_qt
  )
endif()
