# Automatically link Qt executables to qtmain target on Windows
cmake_policy(SET CMP0020 NEW)

file(GLOB MAIN_QT CONFIGURE_DEPENDS
  "../*.rc"
  "../../res/client.qrc"
  "../app_init.cpp"
  "../app_init.h"
  "main.cpp"
)
list(REMOVE_ITEM SOURCES_QT ${MAIN_QT})

# Shared library compiled once and linked into both `client_qt` and
# `client_qt_unittests`. Keeps the heavy `client_application.cpp` and the
# module-wiring support files off the hot path of a test-only rebuild —
# without it, every edit to `main.cpp` or an unrelated test source forces
# these files to be recompiled twice.
add_library(client_qt_app_shared STATIC
  "../client_application.cpp"
  "../client_application.h"
  "../client_application_modules.cpp"
  "../client_application_modules.h"
  "../../modules/opcua_services/opcua_services_module.cpp"
  "../../modules/opcua_services/opcua_services_module.h"
  "../startup_exception.cpp"
  "../startup_exception.h"
  "e2e_test_support.cpp"
  "e2e_test_support.h"
  "startup_flow.cpp"
  "startup_flow.h"
)
target_precompile_headers(client_qt_app_shared REUSE_FROM scada_base)
# opcuapp's include directory must precede the common/ tree so the OPC UA
# services module's `opcua/...` includes resolve to opcuapp.
find_package(opcuapp REQUIRED)
target_include_directories(client_qt_app_shared BEFORE PRIVATE
  $<TARGET_PROPERTY:opcuapp,INTERFACE_INCLUDE_DIRECTORIES>)
if(MSVC)
  target_compile_options(client_qt_app_shared PRIVATE
    /wd4702 # unreachable code in exception/test-support control flow
  )
endif()
target_link_libraries(client_qt_app_shared PUBLIC
  aui_qt
  client_common
  client_configuration_qt
  client_controller_qt
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
)
if(TARGET client_modus_qt)
  target_link_libraries(client_qt_app_shared PUBLIC client_modus_qt)
  target_compile_definitions(client_qt_app_shared PUBLIC CLIENT_HAS_MODUS=1)
endif()
if(TARGET client_vidicon_qt)
  target_link_libraries(client_qt_app_shared PUBLIC client_vidicon_qt)
  target_compile_definitions(client_qt_app_shared PUBLIC CLIENT_HAS_VIDICON=1)
endif()
if(TARGET scada_common_vidicon_services)
  target_compile_definitions(client_qt_app_shared PUBLIC
    CLIENT_HAS_VIDICON_SERVICES=1)
endif()

# Exe

add_executable(client_qt WIN32 ${MAIN_QT})
# Compile res/client.qrc (window icon, tree node icons, toolbar icons). This
# plain executable target does not inherit AUTORCC, so without this the qrc is
# never turned into a Qt resource and every ":/..." lookup is null at runtime.
set_target_properties(client_qt PROPERTIES AUTORCC ON)
target_precompile_headers(client_qt REUSE_FROM scada_base)
target_link_libraries(client_qt PUBLIC client_qt_app_shared)
set_property(TARGET client_qt PROPERTY OUTPUT_NAME "client")
# With a static Qt only the platform plugins Qt considers default for the host
# are linked in — cocoa on macOS, windows on Windows. The offscreen platform is
# not one of them, but the client/server E2E suite runs the client with
# QT_QPA_PLATFORM=offscreen by default (SCADA_E2E_HIDE_CLIENT_WINDOW), so
# without this the client aborts at QApplication construction with "Could not
# find the Qt platform plugin \"offscreen\"" before it writes a single log line
# — every E2E case that launches the client fails identically. The screenshot
# generator renders offscreen for the same reason.
if(COMMAND qt_import_plugins AND TARGET Qt6::QOffscreenIntegrationPlugin)
  qt_import_plugins(client_qt INCLUDE Qt6::QOffscreenIntegrationPlugin)
endif()
if(APPLE)
  set(CLIENT_MACOS_ICON "${CMAKE_CURRENT_SOURCE_DIR}/../../res/client.icns")
  target_sources(client_qt PRIVATE "${CLIENT_MACOS_ICON}")
  set_source_files_properties("${CLIENT_MACOS_ICON}" PROPERTIES
    MACOSX_PACKAGE_LOCATION "Resources")
  set_target_properties(client_qt PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_BUNDLE_NAME "Telecontrol SCADA Client"
    MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
    MACOSX_BUNDLE_GUI_IDENTIFIER "ru.telecontrol.scada.client"
    MACOSX_BUNDLE_ICON_FILE "client.icns"
    MACOSX_BUNDLE_INFO_STRING "Telecontrol SCADA Client"
    MACOSX_BUNDLE_LONG_VERSION_STRING "${PROJECT_VERSION}"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")
endif()
if(MSVC)
  target_link_options(client_qt PRIVATE
    "$<$<CONFIG:Debug>:/DEBUG:NONE>"
    "$<$<CONFIG:Debug>:/INCREMENTAL:NO>"
    "$<$<CONFIG:Debug>:/PDB:NONE>"
  )
endif()
if(COMMAND scada_add_windows_version_info)
  scada_add_windows_version_info(client_qt
    DESCRIPTION "Telecontrol SCADA Client"
    INTERNAL_NAME "SCADA Client"
    ORIGINAL_FILENAME "client.exe")
endif()

# Translations
#
# `client_ru.ts` is the client's whole catalog, deliberately: it is the only
# .ts file qt_add_translation() compiles, so a message in any other file never
# reaches client_ru.qm. Three such files existed until 2026-08-23 (task 384) —
# their translations were correct, present, and unreachable — and a per-module
# registration macro plus an lconvert merge existed to combine them, both dead.
# The check in tools/check_ui_translations.py now fails if a second .ts file
# appears, so this stays a single-catalog arrangement by construction.

find_package(Qt6 COMPONENTS LinguistTools QUIET)
if(COMMAND qt_add_translation)
  qt_add_translation(TRANSLATIONS_OUT "client_ru.ts")

  # Qt's own base catalog (standard QMessageBox buttons, spin/date chrome).
  # The app and the screenshot generator load qtbase_ru.qm at runtime with a
  # fallback to the staged translations dir, so stage it next to client_ru.qm
  # for deployed installs where Qt's own translations path does not exist.
  set(QTBASE_RU_QM
    "${QT6_INSTALL_PREFIX}/${QT6_INSTALL_TRANSLATIONS}/qtbase_ru.qm")
  # A miss here used to be a STATUS line that silently dropped the catalog from
  # STAGED_TRANSLATIONS, which is how the client and every published capture
  # shipped English Qt chrome on all platforms: `qttranslations` was named in
  # no product manifest, so the port was never installed and this lookup never
  # resolved. The port is now in `client/vcpkg.json`'s `$direct`, so an absent
  # catalog means a broken toolchain rather than an ordinary deployment.
  if(NOT EXISTS "${QTBASE_RU_QM}")
    message(FATAL_ERROR
      "qtbase_ru.qm not found at ${QTBASE_RU_QM}. The vcpkg `qttranslations` "
      "port provides it and is declared in client/vcpkg.json; a missing "
      "catalog means the Qt translations were not installed into this build "
      "tree. Re-run `cmake --preset ninja` in client/ to resolve the manifest.")
  endif()

  # Both catalogs are unconditional now that a missing qtbase_ru.qm is a
  # configure error above.
  set(STAGED_TRANSLATIONS
    "${CMAKE_BINARY_DIR}/bin/$<CONFIG>/translations/client_ru.qm"
    "${CMAKE_BINARY_DIR}/bin/$<CONFIG>/translations/qtbase_ru.qm")

  add_custom_command(
    OUTPUT ${STAGED_TRANSLATIONS}
    DEPENDS ${TRANSLATIONS_OUT} ${QTBASE_RU_QM}
    COMMAND ${CMAKE_COMMAND} -E make_directory
      "${CMAKE_BINARY_DIR}/bin/$<CONFIG>/translations"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      ${TRANSLATIONS_OUT} ${QTBASE_RU_QM}
      "${CMAKE_BINARY_DIR}/bin/$<CONFIG>/translations"
    COMMENT "Copy translations to output directory")

  add_custom_target(client_qt_copy_translations
    DEPENDS ${STAGED_TRANSLATIONS})
else()
  add_custom_target(client_qt_copy_translations
    COMMAND ${CMAKE_COMMAND} -E echo
            "Qt LinguistTools were not found; skipping client translations."
    VERBATIM)
endif()

add_dependencies(client_qt client_qt_copy_translations)

if(APPLE AND TRANSLATIONS_OUT)
  # InstalledTranslation loads catalogs from QApplication::applicationDirPath()
  # + "/translations". Inside a bundle that is client.app/Contents/MacOS, not
  # the bin/<config>/translations dir staged above, so the staged copy is
  # invisible to the app and every string silently falls back to its
  # untranslated English source. Mirror the catalogs into the bundle.
  #
  # Driven by the catalogs, not by the link. This was a POST_BUILD command
  # until 2026-08-27, and POST_BUILD runs only when the target *relinks*: a
  # .ts edit regenerates the .qm and re-stages it to bin/<Config>/translations
  # without relinking client_qt, so the bundle — the only path the running app
  # reads — kept whatever catalog the last link left there, and the change was
  # invisible to the running app until something forced a relink. Mirroring
  # client_qt_copy_translations above puts the .qm files in DEPENDS, where the
  # build system can see them.
  # The bundle path is spelled out rather than taken from
  # $<TARGET_BUNDLE_CONTENT_DIR:client_qt>, because an add_custom_command
  # OUTPUT is evaluated with no target in scope and a target-dependent
  # generator expression there fails the generate step outright ("No target
  # client_qt"). $<CONFIG> is fine — it is the one genex OUTPUT documents. The
  # two facts this reconstructs are both set in this file: OUTPUT_NAME is
  # "client", so the bundle is client.app, and CMAKE_RUNTIME_OUTPUT_DIRECTORY
  # is where scada_product_base() puts it.
  set(BUNDLE_TRANSLATIONS_DIR
    "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/client.app/Contents/MacOS/translations")
  set(BUNDLE_TRANSLATIONS
    "${BUNDLE_TRANSLATIONS_DIR}/client_ru.qm"
    "${BUNDLE_TRANSLATIONS_DIR}/qtbase_ru.qm")

  add_custom_command(
    OUTPUT ${BUNDLE_TRANSLATIONS}
    DEPENDS ${TRANSLATIONS_OUT} ${QTBASE_RU_QM}
    COMMAND ${CMAKE_COMMAND} -E make_directory "${BUNDLE_TRANSLATIONS_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      ${TRANSLATIONS_OUT} ${QTBASE_RU_QM} "${BUNDLE_TRANSLATIONS_DIR}"
    COMMENT "Copy translations into the client app bundle")

  add_custom_target(client_qt_copy_bundle_translations
    DEPENDS ${BUNDLE_TRANSLATIONS})
  # Runs before the link rather than after it. The bundle layout does not have
  # to exist yet: make_directory above creates Contents/MacOS/translations, and
  # the linker then writes the executable beside it.
  add_dependencies(client_qt client_qt_copy_bundle_translations)
endif()

# Convenience launcher
#
# CMake presets cannot run anything: there is no "run" preset kind, and a
# workflow step may only be configure, build, test or package
# (https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html; a
# {"type":"run"} step is rejected as `Invalid preset` by cmake 4.4.2, verified
# 2026-08-30). A quick launch command therefore has to be a target reached
# through the build preset that already exists:
#
#   cmake --build --preset relwithdebinfo --target run
#
# $<TARGET_FILE:client_qt> resolves per configuration, so this one target
# serves all three build presets instead of needing one apiece. The name is
# deliberately the bare `run`, matching designer/editor/CMakeLists.txt, so the
# command reads the same from whichever product directory you are in; that is
# safe only because no product this one composes defines a target by that name,
# and a collision would be a hard configure error rather than a silent one.
#
# WORKING_DIRECTORY is the executable's own directory, because that is where
# the build stages what the app reads at runtime -- bin/<Config>/translations
# above, and the Qt plugin set beside the binary on Windows. On macOS it
# resolves to client.app/Contents/MacOS, which is where the bundle catalogs are
# mirrored and what QApplication::applicationDirPath() reports.
#
# On macOS this runs the bundle's inner executable rather than `open`ing the
# .app, which keeps stdout/stderr attached to the terminal that started it.
add_custom_target(run
  COMMAND "$<TARGET_FILE:client_qt>"
  WORKING_DIRECTORY "$<TARGET_FILE_DIR:client_qt>"
  USES_TERMINAL
  VERBATIM)
# add_custom_target's own DEPENDS takes files; a target dependency is
# add_dependencies.
add_dependencies(run client_qt)

if(TRANSLATIONS_OUT)
  if(APPLE)
    # Same reason as the POST_BUILD copy above: the installed bundle must carry
    # its own catalogs, since nothing outside client.app is on the lookup path.
    install(FILES ${TRANSLATIONS_OUT} ${QTBASE_RU_QM}
      DESTINATION "client.app/Contents/MacOS/translations"
      COMPONENT Client)
  else()
    install(FILES ${TRANSLATIONS_OUT} ${QTBASE_RU_QM}
      DESTINATION "bin/translations"
      COMPONENT Client)
  endif()
endif()

install(TARGETS client_qt
  RUNTIME_DEPENDENCY_SET client_deps
  BUNDLE DESTINATION "." COMPONENT Client
  RUNTIME DESTINATION "bin" COMPONENT Client)

if(EXISTS "${CMAKE_SOURCE_DIR}/dependencies/bin")
  install(DIRECTORY "${CMAKE_SOURCE_DIR}/dependencies/bin/"
    DESTINATION "bin"
    COMPONENT Client
    PATTERN "DisplayLib.dll" EXCLUDE)
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/examples/schemes")
  install(DIRECTORY "${CMAKE_SOURCE_DIR}/examples/schemes"
    DESTINATION "examples"
    COMPONENT Client)
endif()

# UT-s

add_executable(client_qt_unittests
  ${UNITTESTS}
  ${UNITTESTS_QT}
)
# ui/qt/client_utils_qt_unittest.cpp checks that every toolbar icon LoadPixmap
# serves resolves from res/client.qrc; compile the resource into the test
# binary (a separate exe from the app, so no duplicate qInitResources_client).
set_target_properties(client_qt_unittests PROPERTIES AUTORCC ON)
target_sources(client_qt_unittests PRIVATE "../../res/client.qrc")
target_precompile_headers(client_qt_unittests REUSE_FROM scada_base)
if(MSVC)
  target_link_options(client_qt_unittests PRIVATE
    "$<$<CONFIG:Debug>:/DEBUG:NONE>"
    "$<$<CONFIG:Debug>:/INCREMENTAL:NO>"
    "$<$<CONFIG:Debug>:/PDB:NONE>"
    "$<$<CONFIG:RelWithDebInfo>:/DEBUG:NONE>"
    "$<$<CONFIG:RelWithDebInfo>:/INCREMENTAL:NO>"
    "$<$<CONFIG:RelWithDebInfo>:/PDB:NONE>"
  )
endif()
# `client_qt_app_shared` transitively supplies the Qt client link set;
# base_unittest provides the gtest main. aui's own tests (aui_qt_unittests,
# aui_test_qt_unittests) are separate binaries owned by aui/.
target_link_libraries(client_qt_unittests PRIVATE
  client_qt_app_shared
  base_unittest
)
include(GoogleTest)
gtest_discover_tests(client_qt_unittests DISCOVERY_MODE PRE_TEST)

# The client_screenshot_generator executable lives in
# `tools/screenshot_generator/` — see that directory's CMakeLists.
