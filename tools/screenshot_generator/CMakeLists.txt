# Offline screenshot generator: renders scada-docs/img/* from a JSON
# fixture. Links against the smaller client_qt_core runtime plus
# client_main_window_qt for the actual top-level shell, and base_unittest
# for the shared gtest main. `client_application.{cpp,h}` is excluded
# from client_qt_core (see app/qt/CMakeLists.txt) so it's compiled in
# directly here — same arrangement as the regular `client_qt` exe.

add_executable(screenshot_generator
  "main.cpp"
  "screenshot_config.cpp"
  "screenshot_config.h"
  "screenshot_output.cpp"
  "screenshot_output.h"
  "widget_capture.cpp"
  "widget_capture.h"
  "graph_capture.cpp"
  "graph_capture.h"
  "dialog_capture.cpp"
  "dialog_capture.h"
  "fixture_builder.cpp"
  "fixture_builder.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/../../app/client_application.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/../../app/client_application.h"
)

target_link_libraries(screenshot_generator PRIVATE
  client_qt_core
  client_main_window_qt
  client_properties_qt
  base_unittest
  graph_qt
)

set_target_properties(screenshot_generator PROPERTIES
  AUTOMOC ON
)

# 32-bit Microsoft link.exe hits LNK1102 ("out of memory") on this
# target — Qt + main-window shell + the full address-space machinery
# (incl. ScadaAddressSpaceBuilder which materialises every standard
# SCADA type) blow past the 4 GB virtual-address ceiling of a 32-bit
# process. Substitute the LLVM lld-link drop-in (64-bit, far smaller
# per-symbol overhead). LINKER_TYPE=LLD tells CMake to invoke lld-link
# instead of link.exe; we prefix PATH so the lookup succeeds even when
# the configure preset doesn't include the LLVM toolset bin dir.
if(MSVC)
  find_program(SCREENSHOT_GENERATOR_LD lld-link
    HINTS
      "C:/Program Files/Microsoft Visual Studio/18/Community/VC/Tools/Llvm/x64/bin"
      "C:/Program Files (x86)/Microsoft Visual Studio/18/Community/VC/Tools/Llvm/x64/bin"
    DOC "Path to lld-link.exe used to link screenshot_generator")
  if(SCREENSHOT_GENERATOR_LD)
    cmake_path(GET SCREENSHOT_GENERATOR_LD PARENT_PATH _llvm_bin)
    cmake_path(NATIVE_PATH _llvm_bin _llvm_bin_native)
    set_property(TARGET screenshot_generator PROPERTY LINKER_TYPE LLD)
    set_property(TARGET screenshot_generator PROPERTY RULE_LAUNCH_LINK
      "\"${CMAKE_COMMAND}\" -E env \"PATH=${_llvm_bin_native};$ENV{PATH}\"")
  endif()
  target_link_options(screenshot_generator PRIVATE
    # Drop the PDB / .ilk and let the linker throw away unreferenced
    # symbols / fold identical funcs. Helpful regardless of linker.
    "/DEBUG:NONE"
    "/INCREMENTAL:NO"
    "/OPT:REF"
    "/OPT:ICF"
  )
endif()

# After the executable links, run it and drop the PNGs into
# client/docs/screenshots/ so the docs gallery stays in sync on every
# local build. The dir is gitignored — commit copies to scada-docs
# separately. Requires a desktop session (see docs/screenshots.md
# "Native window frames").
set(SCREENSHOT_DOCS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../docs/screenshots")
add_custom_command(TARGET screenshot_generator POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory "${SCREENSHOT_DOCS_DIR}"
  COMMAND ${CMAKE_COMMAND} -E env
    "SCREENSHOT_OUT_DIR=${SCREENSHOT_DOCS_DIR}"
    $<TARGET_FILE:screenshot_generator>
  COMMENT "Rendering screenshots into docs/screenshots/"
  VERBATIM
)
