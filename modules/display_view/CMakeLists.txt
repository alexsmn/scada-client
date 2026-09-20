# The client's display surface: a QWidget over a LOADED schematic renderer.
#
# ADR 0013. It links no renderer at all -- `display_runtime.cpp` resolves
# `ScadaDisplayGetApi` out of a shared library at run time, through the C header
# `client_display_abi` supplies. That is what keeps `display`, which is never
# published, out of the public client's source closure.
#
# It used to link `display_view` as source (ADR 0012 phase 3), and before that
# it was `client_vds_runtime`, which dlopened a plugin and blitted BGRA. This is
# neither: the loader is here rather than in a module of its own, and the widget
# it feeds is the one that already existed.
client_module(client_display_view CONFIG qt)

client_module_link_libraries(client_display_view
  PUBLIC
    client_base
    client_common
    # The ABI header, and the dl/LoadLibrary the loader needs. PUBLIC because
    # display_widget.h names the loader's types in its own signatures.
    client_display_abi
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
  PRIVATE
    # Translate() for the operator-facing document and render failures.
    aui
    ${CMAKE_DL_LIBS}
)

# ── Tests ───────────────────────────────────────────────────────────────────

# The loader's suite drives the REAL library when the configure named one, which
# is the only way to test the client's half of the boundary: the copy-out of the
# ABI's borrowed strings, the version negotiation, the search order. The library
# and the fixture reach the test as definitions rather than as a staged file,
# because a test binary's own directory is not where the packaging puts a runtime
# and staging one per test target would be a copy apiece.
#
# An empty definition is the configuration with no runtime, and the cases that
# need one skip with that said out loud.
if(TARGET client_display_view_qt_unittests)
  target_compile_definitions(client_display_view_qt_unittests PRIVATE
    SCADA_DISPLAY_RUNTIME_UNDER_TEST="${SCADA_DISPLAY_RUNTIME_LIBRARY}"
    SCADA_DISPLAY_TEST_DOCUMENT="${CMAKE_CURRENT_SOURCE_DIR}/../../tools/screenshot_generator/fixtures/substation.vds"
  )
endif()
