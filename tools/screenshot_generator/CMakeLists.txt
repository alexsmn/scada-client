# Offline screenshot generator: renders doc screenshots from a JSON
# fixture. It should ride the same client-side dependency stack as the
# real Qt application, plus the shared gtest runner used by the Qt test
# binaries. `client_application.{cpp,h}` is excluded from the reusable
# base-owned helper target, so it and the shared app-support wiring are
# compiled in directly here — same arrangement as the regular `client_qt` exe.

find_package(Boost REQUIRED COMPONENTS program_options)

file(GLOB SCREENSHOT_GENERATOR_WIN32_RESOURCES CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/../../app/*.rc"
)

add_executable(client_screenshot_generator
  "main.cpp"
  "control_confirmation_check.cpp"
  "explorer_selection.cpp"
  "explorer_selection.h"
  "menu_capture.cpp"
  "settings_panel_capture.cpp"
  "standalone_captures.cpp"
  "standalone_captures.h"
  "workbench_capture.cpp"
  "screenshot_config.cpp"
  "screenshot_config.h"
  "screenshot_options.cpp"
  "screenshot_options.h"
  "screenshot_output.cpp"
  "screenshot_output.h"
  "screenshot_wait.cpp"
  "screenshot_wait.h"
  "screenshot_modules.cpp"
  "screenshot_modules.h"
  "widget_capture.cpp"
  "widget_capture.h"
  "publish_guard.cpp"
  "publish_guard.h"
  "view_capture.cpp"
  "view_capture.h"
  "graph_capture.cpp"
  "graph_capture.h"
  "device_diagnostics_capture.cpp"
  "device_metrics_capture.cpp"
  "device_metrics_capture.h"
  "administration_capture.cpp"
  "user_access_capture.cpp"
  "transmission_rule_capture.cpp"
  "bulk_create_capture.cpp"
  "bulk_create_capture.h"
  "severity_tiles_capture.cpp"
  "severity_tiles_capture.h"
  "command_field_capture.cpp"
  "command_field_capture.h"
  "shell_strip_capture.cpp"
  "shell_strip_capture.h"
  "inspector_capture.cpp"
  "inspector_capture.h"
  "transmission_rule_capture.h"
  "administration_capture.h"
  "user_access_capture.h"
  "device_diagnostics_capture.h"
  "dialog_capture.cpp"
  "dialog_capture.h"
  "dialog_grab.cpp"
  "dialog_grab.h"
  "dialog_kinds.h"
  "dialog_node_captures.cpp"
  "dialog_pump.h"
  "dialog_shell_captures.cpp"
  "dialog_write_captures.cpp"
  "debugger_capture.cpp"
  "debugger_capture.h"
  "frame_decode_capture.cpp"
  "frame_decode_capture.h"
  "fixture_model_assertions.cpp"
  "shell_assertions.cpp"
  "write_dialog_assertions.cpp"
  "fixture_builder.cpp"
  "fixture_users.cpp"
  "fixture_builder.h"
  "screenshot_fixture.cpp"
  "screenshot_fixture.h"
  ${SCREENSHOT_GENERATOR_WIN32_RESOURCES}
  "${CMAKE_CURRENT_SOURCE_DIR}/../../modules/opcua_services/opcua_services_module.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/../../modules/opcua_services/opcua_services_module.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/../../app/client_application.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/../../app/client_application.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/../../app/client_application_modules.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/../../app/client_application_modules.h"
)

# Compile client.qrc so LoadPixmap's :/res toolbar icons resolve in the
# generated screenshots, matching the client_qt app. Separate exe from the
# app, so no duplicate qInitResources_client symbol.
set_target_properties(client_screenshot_generator PROPERTIES AUTORCC ON)
target_sources(client_screenshot_generator PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/../../res/client.qrc")

# opcuapp include precedence (see client/app/qt/CMakeLists.txt).
find_package(opcuapp REQUIRED)
target_include_directories(client_screenshot_generator BEFORE PRIVATE
  $<TARGET_PROPERTY:opcuapp,INTERFACE_INCLUDE_DIRECTORIES>)

target_link_libraries(client_screenshot_generator PRIVATE
  aui_qt
  client_common
  client_configuration_qt
  client_controller_qt
  client_device_diagnostics_qt
  client_inspector_qt
  client_administration_qt
  client_user_access_qt
  client_component_transmission_qt
  client_transmission_rules_qt
  client_bulk_create_qt
  client_events_qt
  client_export_configuration_qt
  client_export_excel_qt
  client_graph_qt
  client_profile_qt
  client_services_qt
  ${CLIENT_QT_COMPONENT_TARGETS}
  client_ui_common_qt
  client_ui_dragdrop_qt
  client_ui_progress_qt
  client_export_csv_qt
  client_favorites_qt
  client_main_window_qt
  client_portfolio_qt
  client_print_service_qt
  client_properties_qt
  scada_core_opcua_client
  Boost::program_options
  base_unittest
  graph_qt
)
# The display captures, following the same shape as modus/vidicon below: the
# target exists only when CLIENT_WITH_DISPLAY is on (ADR 0013 phase 1), and
# display_capture.cpp is what draws through it.
if(TARGET client_display_view_qt)
  target_sources(client_screenshot_generator PRIVATE
    "display_capture.cpp"
    "display_capture.h")
  target_link_libraries(client_screenshot_generator PRIVATE
    client_display_frame_qt
    client_display_view_qt)
  target_compile_definitions(client_screenshot_generator PRIVATE CLIENT_HAS_DISPLAY=1)
endif()
if(TARGET client_modus_qt)
  target_link_libraries(client_screenshot_generator PRIVATE client_modus_qt)
  target_compile_definitions(client_screenshot_generator PRIVATE CLIENT_HAS_MODUS=1)
endif()
if(TARGET client_vidicon_qt)
  target_link_libraries(client_screenshot_generator PRIVATE client_vidicon_qt)
  target_compile_definitions(client_screenshot_generator PRIVATE CLIENT_HAS_VIDICON=1)
endif()

add_dependencies(client_screenshot_generator client_qt_copy_translations)

set_target_properties(client_screenshot_generator PROPERTIES
  AUTOMOC ON
)

# This generator only ever runs on the offscreen platform — every ctest
# registration below and the macOS gallery run set QT_QPA_PLATFORM=offscreen —
# but with a static Qt only the host's default platform plugins are linked in
# (cocoa on macOS, windows on Windows). Without naming it here the executable
# aborts in QApplication's constructor with `Could not find the Qt platform
# plugin "offscreen"`, so the checks below fail having captured nothing. Same
# requirement, same fix as `client_qt` (client/app/qt/CMakeLists.txt).
if(COMMAND qt_import_plugins AND TARGET Qt6::QOffscreenIntegrationPlugin)
  qt_import_plugins(client_screenshot_generator
    INCLUDE Qt6::QOffscreenIntegrationPlugin)
endif()

if(MSVC)
  target_link_options(client_screenshot_generator PRIVATE
    $<$<CONFIG:Debug>:/DEBUG:NONE>
    $<$<CONFIG:Debug>:/INCREMENTAL:NO>
    $<$<CONFIG:Debug>:/PDB:NONE>)
endif()

# Unit tests for the generator's own logic.
#
# `client/CMakeLists.txt` globs the client tree for `*_unittest.cpp` but
# explicitly excludes `/client/tools/`, because each tool manages its own
# sources. Nothing here defined a test target, so until 2026-08-25 every
# `*_unittest.cpp` under this directory was compiled by nothing at all --
# `screenshot_wait_unittest.cpp` had four cases that had never run. A test
# file that no target builds looks exactly like a passing one, so keep this
# target in step when adding another.
#
# Deliberately nothing Qt: what it covers is the settle loop's budget
# arithmetic, the publish guard's failure-count scoping and the command-line
# parser, all Qt-free on purpose so they can be tested without a widget, an
# event loop, or a machine that happens to be idle. `Boost::program_options` is
# the one non-gtest link, and it is there because the parser is what it tests.
add_executable(client_screenshot_generator_unittests
  "settle_loop_unittest.cpp"
  "publish_guard.cpp"
  "publish_guard_unittest.cpp"
  "screenshot_options.cpp"
  "screenshot_options_unittest.cpp"
  "screenshot_output.cpp"
  "screenshot_output_unittest.cpp"
)

target_link_libraries(client_screenshot_generator_unittests PRIVATE
  base_unittest
  Boost::program_options
)

target_include_directories(client_screenshot_generator_unittests PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
)

include(GoogleTest)
gtest_discover_tests(client_screenshot_generator_unittests
  DISCOVERY_MODE PRE_TEST PROPERTIES TIMEOUT 60)

set_property(TARGET client_screenshot_generator_unittests PROPERTY FOLDER
  ${scada_client_folder})

# Structural regression net: run the generator on the Qt offscreen
# platform into a scratch dir and verify every manifest-managed capture
# exists with spec-exact dimensions (see check_screenshots.py). Rides
# ctest, so any CI that runs the test suite exercises the docs pipeline.
find_package(Python3 COMPONENTS Interpreter)
if(Python3_FOUND)
  add_test(NAME client_screenshot_check
    COMMAND ${Python3_EXECUTABLE}
      "${CMAKE_CURRENT_SOURCE_DIR}/check_screenshots.py"
      "--generator=$<TARGET_FILE:client_screenshot_generator>"
      "--data=${CMAKE_CURRENT_SOURCE_DIR}/screenshot_data.json"
      "--image-manifest=${CMAKE_CURRENT_SOURCE_DIR}/../../screenshots/image_manifest.json"
      "--out=${CMAKE_CURRENT_BINARY_DIR}/screenshot_check"
  )
  set_tests_properties(client_screenshot_check PROPERTIES TIMEOUT 300)

  # The owed-set query the check above reports with — which manifest-managed
  # rows no capture in this generator produces. Its own tests, because the
  # figure it replaces was derived by hand and was wrong twice in the same
  # direction: a capture rendering from a filename hardcoded in C++ rather than
  # named in the fixture went on being counted as owed. Runs in milliseconds
  # and needs no build: the cases carry their own manifest and sources.
  add_test(NAME client_screenshot_owed_tests
    COMMAND ${Python3_EXECUTABLE}
      "${CMAKE_CURRENT_SOURCE_DIR}/test_check_screenshots.py")
  set_tests_properties(client_screenshot_owed_tests PROPERTIES TIMEOUT 120)

  # Capture provenance (task 434). Like the owed-set tests: milliseconds, no
  # build, no Qt, no generator binary - each case builds its own manifest and
  # images, and the git-dependent checks run against a throwaway repository.
  add_test(NAME client_screenshot_provenance_tests
    COMMAND ${Python3_EXECUTABLE}
      "${CMAKE_CURRENT_SOURCE_DIR}/test_capture_provenance.py")
  set_tests_properties(client_screenshot_provenance_tests PROPERTIES TIMEOUT 120)
endif()

# Without an interpreter the three checks above register nothing, and a suite
# that checked no capture at all reports green. That is the shape
# `scada_orphan_unittests_check` exists for one level up: a test that never runs
# looks exactly like a passing one, and it is invisible precisely on the hosts
# that need telling. So say it as a failure rather than as silence.
#
# A failing placeholder rather than a hard configure error: the client must
# still build on a host with no Python, and this is a gap in *verification*, not
# in the build. Deliberately not decided by whether this workstation has an
# interpreter (backlog 632).
#
# The script is generated into the build tree rather than tracked, so the reason
# lives here beside the guard it explains instead of in a second file that can
# drift from it.
if(NOT Python3_FOUND)
  set(screenshot_no_python_script
    "${CMAKE_CURRENT_BINARY_DIR}/screenshot_checks_unavailable.cmake")
  file(WRITE "${screenshot_no_python_script}"
    "message(FATAL_ERROR\n"
    "  \"No Python 3 interpreter was found when this build tree was \"\n"
    "  \"configured, so client_screenshot_check, \"\n"
    "  \"client_screenshot_owed_tests and \"\n"
    "  \"client_screenshot_provenance_tests were not registered and no \"\n"
    "  \"screenshot has been checked. Install Python 3 and re-configure; \"\n"
    "  \"this test exists so their absence is loud rather than silent \"\n"
    "  \"(backlog 632).\")\n")
  add_test(NAME client_screenshot_checks_unavailable
    COMMAND ${CMAKE_COMMAND} -P "${screenshot_no_python_script}")
endif()

# Regenerating the tracked gallery is an explicit request, never a build side
# effect. `client/screenshots/` is committed (a UI change lands as a reviewable
# image diff), and this used to be a POST_BUILD step that rewrote those PNGs on
# every link — so any build of any client target dirtied tracked files, in
# whatever worktree happened to be building. On macOS the renders differ from
# the Windows baseline anyway (offscreen Qt rasterizes with the host's fonts),
# so the churn was never a real UI diff, only noise that risked being committed
# as one. Regeneration now rides the `regenerate_client_screenshots` target,
# and the smoke path stays covered on every build by `client_screenshot_check`
# above, which renders into the build tree.
set(SCREENSHOT_DOCS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../screenshots")

# On macOS force the offscreen platform: the local gallery is
# validation-only there (published images come from Windows), and cocoa
# runs from headless/background build contexts have livelocked in event
# dispatch, wedging the build at 100% CPU. Windows keeps the native
# platform so the published gallery renders with real fonts.
if(APPLE)
  set(SCREENSHOT_CAPTURE_LAUNCHER
      ${CMAKE_COMMAND} -E env QT_QPA_PLATFORM=offscreen)
else()
  set(SCREENSHOT_CAPTURE_LAUNCHER)
endif()

# The gallery is rendered by a single pass, under the design-token appearance
# the generator defaults to (dark). There used to be two: a default pass that
# rendered the untouched platform look, then a second `--theme=dark` pass that
# re-rendered just the published subset on top of it, plus a `publish_theme`
# manifest field and a copy_themed_captures.cmake to police the overwrite. All
# of that existed because the client shipped an un-themed "Classic" appearance
# and half the gallery documented it. It no longer does, so one pass renders
# what the client actually looks like.
set(SCREENSHOT_IMAGE_MANIFEST "${SCREENSHOT_DOCS_DIR}/image_manifest.json")
# Collapse the `../..` in SCREENSHOT_DOCS_DIR: CMAKE_CONFIGURE_DEPENDS matches
# on the path it is given, so an unnormalized one can miss the re-configure.
get_filename_component(SCREENSHOT_IMAGE_MANIFEST
  "${SCREENSHOT_IMAGE_MANIFEST}" ABSOLUTE)
set_property(DIRECTORY APPEND PROPERTY
  CMAKE_CONFIGURE_DEPENDS "${SCREENSHOT_IMAGE_MANIFEST}")

# Record, per regenerated image, the commit and platform it was captured at
# (task 434). A screenshot is a claim about the tree that produced it, and until
# this ran nothing in the pipeline recorded which tree that was — so a stale
# capture was silent and the gallery still read as a baseline. The stamp goes
# last, after the render pass, so it sees the bytes that were actually
# published; it rewrites only rows whose digest changed, because an unchanged
# image is still the output of the older commit already recorded against it.
if(Python3_FOUND)
  set(screenshot_provenance_stamp
    COMMAND ${Python3_EXECUTABLE}
      "${CMAKE_CURRENT_SOURCE_DIR}/capture_provenance.py"
      "--stamp"
      "--image-manifest=${SCREENSHOT_IMAGE_MANIFEST}"
      "--images-dir=${SCREENSHOT_DOCS_DIR}")
endif()

add_custom_target(regenerate_client_screenshots
  COMMAND ${CMAKE_COMMAND} -E make_directory "${SCREENSHOT_DOCS_DIR}"
  COMMAND ${SCREENSHOT_CAPTURE_LAUNCHER} $<TARGET_FILE:client_screenshot_generator>
    "--out=${SCREENSHOT_DOCS_DIR}"
    "--image-manifest=${SCREENSHOT_IMAGE_MANIFEST}"
  ${screenshot_provenance_stamp}
  COMMENT "Rendering screenshots into client/screenshots/"
  VERBATIM
  USES_TERMINAL
)
add_dependencies(regenerate_client_screenshots client_screenshot_generator)
