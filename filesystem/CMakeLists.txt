client_module(client_filesystem)

client_module_link_libraries(client_filesystem PUBLIC
  scada_common
)

client_module_link_libraries(client_filesystem PRIVATE
  client_controller
)

list(APPEND UNITTESTS
  "${CMAKE_CURRENT_SOURCE_DIR}/file_cache_unittest.cpp"
)
set(UNITTESTS "${UNITTESTS}" PARENT_SCOPE)
