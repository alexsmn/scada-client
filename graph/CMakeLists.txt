client_module(client_graph CONFIG qt)

find_package(GraphQt REQUIRED)

client_module_link_libraries(client_graph PRIVATE
  client_controller
  graph
  timed_data
)

target_link_libraries(client_graph_qt_unittests PRIVATE
  client_controller_test_qt
)