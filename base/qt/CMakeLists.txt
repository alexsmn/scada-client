scada_module(client_base_qt)

target_link_libraries(client_base_qt PUBLIC client_base)

find_package(Qt5 COMPONENTS Core REQUIRED)

target_link_libraries(client_base_qt PUBLIC Qt5::Core)
