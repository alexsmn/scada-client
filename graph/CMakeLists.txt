client_module(client_graph CONFIG qt)

# TODO: Rework with `find_package`.
add_subdirectory("${deps}/qt/graph_qt" "graph_qt") 

client_module_link_libraries(client_graph PRIVATE
  client_controller
  graph
  timed_data
)

target_link_libraries(client_graph_qt_unittests PRIVATE
  client_controller_test_qt
)