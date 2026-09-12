client_module(client_modus CONFIG qt)

# The version-2 (`.xsde`) and ActiveX renderers.
#
# Task 484: `activex/` and `libmodus/` were compiled by nothing. `client_module`
# globs the module root and its `qt/` subdirectory only — `_scada_module_add_sources`
# is non-recursive — so `libclient_modus_qt.a` held three object files, and
# `activex/modus_document_unittest.cpp` was a 147-line test that no target had
# ever built. A test file nothing builds looks exactly like a passing one.
#
# Task 483 settled that the version-2 path is a live product direction, so these
# are wired up here rather than deleted. They are opt-in because neither can be
# built in this tree as it stands:
#
#  * `activex/` is Windows-only COM/ATL against the Modus 6.30 SDK, and has
#    never been through a compiler. Adding ~3000 lines of that to the Windows
#    build unconditionally would very likely break it for everyone, and there is
#    nowhere in this tree to verify it first (ADR 0011 phase 1's Windows exit
#    criterion is still outstanding).
#  * `libmodus/` is glue to a `libmodus` SDK that is NOT vendored here:
#    `qt/modus_view2.cpp` includes `libmodus/gfx/canvas.h`, `libmodus/render/*`
#    and `libmodus/scheme/*`, none of which exist anywhere in the tree. It
#    cannot compile until that SDK is vendored or found.
#
# So: set CLIENT_MODUS_ACTIVEX=ON on a Windows machine holding the Modus 6.30
# SDK to build the ActiveX renderer and, for the first time, its unit test.
# CLIENT_MODUS_LIBMODUS additionally needs the libmodus SDK on the include path.
option(CLIENT_MODUS_ACTIVEX
  "Build the Modus ActiveX renderer (Windows + Modus 6.30 SDK)" OFF)
option(CLIENT_MODUS_LIBMODUS
  "Build the Modus version-2 renderer (needs the libmodus SDK)" OFF)

if(CLIENT_MODUS_ACTIVEX AND NOT WIN32)
  message(FATAL_ERROR
    "CLIENT_MODUS_ACTIVEX is Windows-only: activex/ is COM/ATL against the "
    "Modus 6.30 SDK.")
endif()

if(CLIENT_MODUS_ACTIVEX)
  client_module_sources(client_modus PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/activex")
endif()

if(CLIENT_MODUS_LIBMODUS)
  client_module_sources(client_modus PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/libmodus")
endif()

# `qt/modus_view.cpp` is the ActiveX view and `qt/modus_view2.cpp` the libmodus
# one; each is excluded unless its renderer is being built. Production uses
# neither — `ModusController` builds a `ModusDisplayView` over the linked
# `display` product — so with both options OFF this is the pre-484 source list
# exactly.
if(TARGET client_modus_qt)
  set(_client_modus_excluded_views)
  if(NOT CLIENT_MODUS_ACTIVEX)
    list(APPEND _client_modus_excluded_views "qt/modus_view\\.(cpp|h)$")
  endif()
  if(NOT CLIENT_MODUS_LIBMODUS)
    list(APPEND _client_modus_excluded_views "qt/modus_view2\\.(cpp|h)$")
  endif()

  if(_client_modus_excluded_views)
    list(JOIN _client_modus_excluded_views "|" _client_modus_excluded_views_re)
    get_target_property(_client_modus_qt_sources client_modus_qt SOURCES)
    list(FILTER _client_modus_qt_sources EXCLUDE REGEX
      "${_client_modus_excluded_views_re}")
    set_target_properties(client_modus_qt PROPERTIES
      SOURCES "${_client_modus_qt_sources}")
  endif()
endif()

client_module_link_libraries(client_modus
  PUBLIC
    # PUBLIC because modus_util.h and modus_view_wrapper.h name
    # scada::display::view::DocumentKind in their own signatures: since ADR 0012
    # phase 3 the renderer is linked, so the enum travels instead of the int32_t
    # that used to keep the plugin's C API out of those headers.
    client_display_view
  PRIVATE
    client_controller
)

if(TARGET client_modus_qt_unittests)
  target_link_libraries(client_modus_qt_unittests PRIVATE
    aui_qt
    client_filesystem_qt
  )
endif()
