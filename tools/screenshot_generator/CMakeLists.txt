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
  "view_capture.cpp"
  "view_capture.h"
  "graph_capture.cpp"
  "graph_capture.h"
  "display_capture.cpp"
  "display_capture.h"
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
  "inspector_capture.cpp"
  "inspector_capture.h"
  "transmission_rule_capture.h"
  "administration_capture.h"
  "user_access_capture.h"
  "device_diagnostics_capture.h"
  "dialog_capture.cpp"
  "dialog_capture.h"
  "debugger_capture.cpp"
  "debugger_capture.h"
  "frame_decode_capture.cpp"
  "frame_decode_capture.h"
  "fixture_assertions.cpp"
  "fixture_builder.cpp"
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
  client_display_frame_qt
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
  client_vds_runtime_qt
  scada_core_opcua_client
  Boost::program_options
  base_unittest
  graph_qt
)
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

# Themed reshell-capture regression net. The structural check above renders the
# legacy (no-theme) managed subset, so it never exercises the theme-gated
# reshell captures (the "reshell-theme"-tagged manifest entries): the
# config-workbench tabs that click a subtab (config-address-map, config-limits)
# and the explorer trees whose status dots / filter only appear under --theme
# (hardware-tree, object-tree). Render them together under --theme so a
# regression fails the suite — in particular the per-spec view-selection bug
# where every "NewProps" spec re-grabbed the first opened view, pointing
# config-limits at TS.702's form (no Limits subtab). The generator is a gtest
# binary, so any ADD_FAILURE (e.g. "click_object not found") returns non-zero
# and fails ctest. Keep this --only list in sync with the reshell-theme entries
# in screenshots/image_manifest.json.
add_test(NAME client_screenshot_check_themed
  COMMAND ${CMAKE_COMMAND} -E env QT_QPA_PLATFORM=offscreen
    $<TARGET_FILE:client_screenshot_generator>
    "--theme=dark"
    "--only=administration-explorer.png,config-parameters.png,config-address-map.png,config-limits.png,hardware-tree.png,object-tree.png,users-admin.png,admin-simulation.png,admin-formats.png,admin-historical-dbs.png,severity-tiles.png,command-field.png,events-alarm-surface.png,table-workspace.png,workbench-window.png,workbench-overview.png,workbench-activity-rail.png,workbench-login.png,substation-display.png,inspector-event.png,transmission-rules.png,series-inspector.png,roles.png,password-policy.png"
    "--out=${CMAKE_CURRENT_BINARY_DIR}/screenshot_check_themed"
)
set_tests_properties(client_screenshot_check_themed PROPERTIES TIMEOUT 300)

# Regenerating the tracked gallery is an explicit request, never a build side
# effect. `client/screenshots/` is committed (a UI change lands as a reviewable
# image diff), and this used to be a POST_BUILD step that rewrote those PNGs on
# every link — so any build of any client target dirtied tracked files, in
# whatever worktree happened to be building. On macOS the renders differ from
# the Windows baseline anyway (offscreen Qt rasterizes with the host's fonts),
# so the churn was never a real UI diff, only noise that risked being committed
# as one. Regeneration now rides the `regenerate_client_screenshots` target,
# and the smoke path stays covered on every build by the two ctest tests above,
# which render into the build tree.
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

# The published images render dark; the rest of the gallery stays legacy.
#
# Both themes are kept on purpose (decided 2026-08-02). The default no-theme
# run remains the regression net for the non-reshell client — that is what an
# operator on Classic actually sees — while the handful of images the web
# manual ships render under the reshell tokens, so the manual reads as one
# consistent product instead of alternating light and dark page to page.
#
# Which files those are is data, not a list to keep in sync by hand: an image
# is published iff it is in `current_generator_owned_subset`, and it renders
# dark unless its manifest entry says `"publish_theme": "legacy"`. Read at
# configure time, so a manifest edit re-runs CMake (CMAKE_CONFIGURE_DEPENDS
# below) rather than silently publishing the old set.
set(SCREENSHOT_IMAGE_MANIFEST "${SCREENSHOT_DOCS_DIR}/image_manifest.json")
# Collapse the `../..` in SCREENSHOT_DOCS_DIR: CMAKE_CONFIGURE_DEPENDS matches
# on the path it is given, so an unnormalized one can miss the re-configure.
get_filename_component(SCREENSHOT_IMAGE_MANIFEST
  "${SCREENSHOT_IMAGE_MANIFEST}" ABSOLUTE)
set_property(DIRECTORY APPEND PROPERTY
  CMAKE_CONFIGURE_DEPENDS "${SCREENSHOT_IMAGE_MANIFEST}")
file(READ "${SCREENSHOT_IMAGE_MANIFEST}" screenshot_manifest_json)

# Map each published filename onto its publish theme. CMake's JSON support has
# no lookup-by-value, so this scans the images array per published file; the
# subset is single digits and the array under a hundred, so the cost is noise.
set(SCREENSHOT_PUBLISH_DARK)
string(JSON published_count LENGTH
  "${screenshot_manifest_json}" current_generator_owned_subset)
string(JSON manifest_image_count LENGTH "${screenshot_manifest_json}" images)
if(published_count GREATER 0)
  math(EXPR published_last "${published_count} - 1")
  math(EXPR manifest_image_last "${manifest_image_count} - 1")
  foreach(published_index RANGE ${published_last})
    string(JSON published_file GET "${screenshot_manifest_json}"
      current_generator_owned_subset ${published_index})

    # Absent `publish_theme` means dark: the manual's default is the reshell
    # look, and an exemption has to be written down to take effect.
    set(publish_theme "dark")
    foreach(image_index RANGE ${manifest_image_last})
      string(JSON entry_file GET "${screenshot_manifest_json}"
        images ${image_index} file)
      if(entry_file STREQUAL published_file)
        string(JSON entry_theme ERROR_VARIABLE entry_theme_error
          GET "${screenshot_manifest_json}" images ${image_index} publish_theme)
        if(NOT entry_theme_error)
          set(publish_theme "${entry_theme}")
        endif()
        break()
      endif()
    endforeach()

    if(publish_theme STREQUAL "dark")
      list(APPEND SCREENSHOT_PUBLISH_DARK "${published_file}")
    endif()
  endforeach()
endif()

# Second pass: re-render just those files under the theme and copy them over
# the legacy versions the first pass wrote. Order matters — the full run has to
# go first, or it would paint the published images back to legacy. The pass
# renders into the build tree and copy_themed_captures.cmake does the copy, so
# a capture that skips under --theme fails the target instead of leaving the
# legacy render in place; see that script's header.
set(screenshot_dark_publish_pass)
if(SCREENSHOT_PUBLISH_DARK)
  list(JOIN SCREENSHOT_PUBLISH_DARK "," screenshot_publish_dark_only)
  set(screenshot_publish_dark_dir
    "${CMAKE_CURRENT_BINARY_DIR}/screenshot_publish_dark")
  set(screenshot_dark_publish_pass
    COMMAND ${CMAKE_COMMAND} -E rm -rf "${screenshot_publish_dark_dir}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${screenshot_publish_dark_dir}"
    COMMAND ${SCREENSHOT_CAPTURE_LAUNCHER}
      $<TARGET_FILE:client_screenshot_generator>
      "--theme=dark"
      "--only=${screenshot_publish_dark_only}"
      "--out=${screenshot_publish_dark_dir}"
      "--image-manifest=${SCREENSHOT_IMAGE_MANIFEST}"
    COMMAND ${CMAKE_COMMAND}
      -DSCADA_THEMED_SRC_DIR=${screenshot_publish_dark_dir}
      -DSCADA_THEMED_DEST_DIR=${SCREENSHOT_DOCS_DIR}
      -DSCADA_THEMED_FILES=${screenshot_publish_dark_only}
      -P "${CMAKE_CURRENT_SOURCE_DIR}/copy_themed_captures.cmake")
endif()

# Record, per regenerated image, the commit and platform it was captured at
# (task 434). A screenshot is a claim about the tree that produced it, and until
# this ran nothing in the pipeline recorded which tree that was — so a stale
# capture was silent and the gallery still read as a baseline. The stamp goes
# last, after both render passes, so it sees the bytes that were actually
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
  ${screenshot_dark_publish_pass}
  ${screenshot_provenance_stamp}
  COMMENT "Rendering screenshots into client/screenshots/"
  VERBATIM
  USES_TERMINAL
)
add_dependencies(regenerate_client_screenshots client_screenshot_generator)
