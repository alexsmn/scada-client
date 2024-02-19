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

file(GLOB_RECURSE MODUS_IDLS CONFIGURE_DEPENDS
  "."
  "*.idl"
)

# target_sources(client_modus_qt PRIVATE ${MODUS_IDLS})
# find_package(MIDL)
# build_idls(TARGET client_modus_qt)

target_link_libraries(client_modus_qt PRIVATE
  gdiplus
  modus
  schematic
)

add_subdirectory(tester)
