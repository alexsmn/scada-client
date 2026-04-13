client_module(client_print_service)
client_module_sources(client_print_service PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

client_module_link_libraries(client_print_service PUBLIC aui)

# Qt.

find_package(Qt5 COMPONENTS PrintSupport REQUIRED)

target_link_libraries(client_print_service_qt PUBLIC
  Qt5::PrintSupport
)
