add_library(client_modus_qt)
scada_module_sources(client_modus_qt PRIVATE "." "activex" "libmodus" "qt")

add_subdirectory("${deps}/modus" modus)
target_compile_definitions(modus PUBLIC MODUS_GDIP)

add_subdirectory("${deps}/qt/schematic" "schematic") 

target_include_directories(client_modus_qt PRIVATE
  "${third_party}/modus"
)

target_link_libraries(client_modus_qt PRIVATE
  client_controller_qt
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
