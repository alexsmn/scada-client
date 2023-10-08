cient_module(client_graph)

# QT.

# TODO: Rework with `find_package`.
add_subdirectory("${deps}/qt/graph_qt" "graph_qt") 

target_link_libraries(client_graph_qt PRIVATE
  client_controller_qt
  graph_qt
)
