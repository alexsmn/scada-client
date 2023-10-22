client_module(client_modus CONFIG qt)
client_module_sources(client_modus PRIVATE activex libmodus)

client_module_link_libraries(client_modus PRIVATE
  client_controller
  client_web
)

add_subdirectory("${deps}/modus" modus)
target_compile_definitions(modus PUBLIC MODUS_GDIP)

add_subdirectory("${deps}/qt/schematic" "schematic") 

target_include_directories(client_modus_qt PRIVATE
  "${third_party}/modus"
)

target_link_libraries(client_modus_qt PRIVATE
  gdiplus
  modus
  schematic
)

# Expose component publicly to link it correctly.

target_sources(client_modus_qt INTERFACE
  "modus_component.cpp"
  "modus_component.h"
)

add_subdirectory(tester)
