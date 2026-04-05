client_module(client_modus CONFIG qt)
client_module_sources(client_modus PRIVATE activex libmodus)

client_module_link_libraries(client_modus PRIVATE
  client_controller
  client_web
)

find_package(Modus REQUIRED)
target_compile_definitions(modus PUBLIC MODUS_GDIP)

find_package(Schematic REQUIRED)

file(GLOB_RECURSE MODUS_IDLS CONFIGURE_DEPENDS
  "."
  "*.idl"
)

# target_sources(client_modus_qt PRIVATE ${MODUS_IDLS})
# find_package(MIDL)
# build_idls(TARGET client_modus_qt)

# Disable ccache for modus: #import generates .tlh/.tli files as side effects
# that ccache cannot track.
set_target_properties(client_modus_qt PROPERTIES
  C_COMPILER_LAUNCHER ""
  CXX_COMPILER_LAUNCHER ""
)

target_link_libraries(client_modus_qt PRIVATE
  gdiplus
  modus
  schematic
)

# UTs

target_link_libraries(client_modus_qt_unittests PRIVATE
  client_filesystem_qt
)

# Tester

add_subdirectory(tester)
