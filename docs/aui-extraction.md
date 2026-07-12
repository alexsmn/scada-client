# AUI extraction readiness

`aui/` (the abstract UI layer: platform-agnostic models plus the Qt/Wt view
wrappers) is slated for extraction into its own repository. This document
records the decoupling that prepared it, the dependency contract it must keep,
and the mechanical steps that remain for the actual split.

## Dependency contract

`aui` may depend on, and only on:

| Dependency | Provides | Repository |
|---|---|---|
| `scada_base` | `base/*` utilities (check, awaitable, pickle, formats, …) | scada-core |
| `graph_qt` | `views::Graph*` widgets wrapped by `aui/qt/graph.h` | graph-qt |
| `view_manager_qt` | `ViewManagerQtComponent` wrapped by `aui/view_manager.h` | view_manager_qt |
| `Qt6::Widgets` (+ `AxContainer` on Windows) | Qt flavor (`aui_qt`) | — |
| `Wt::Wt` | Wt flavor (`aui_wt`) | — |

Everything else is a layering violation. In particular, **nothing under
`aui/` may include headers from the client repo** (`profile/`, `resources/`,
`ui/`, `modules/`, `main_window/`, …) or from scada-common. When aui code
needs something client-specific, invert the dependency: keep the generic seam
in aui and move the client-coupled piece to its consumer (see the moves
below for the pattern).

`graph_qt` and `view_manager_qt` are deliberate dependencies, not warts: the
`aui/graph.h` / `aui/view_manager.h` switcher headers are the same
abstract-UI seam pattern as `aui/table.h` / `aui/tree.h`, and both component
libraries are standalone repos an extracted aui can depend on like Qt/Wt.
`aui_qt` declares them itself (find modules resolved through the
preset-seeded `CMAKE_MODULE_PATH`) instead of borrowing include paths from
consumer targets as it did historically.

## Decoupling performed (2026-07)

- **Dead cross-repo includes removed** — `profile/window_definition_util.h`
  from `aui/qt/table.cpp` and `aui/wt/table.cpp`, `net/net_executor_adapter.h`
  from `aui/qt/dialog_util.h`. None of their symbols were used.
- **`aui/dragdrop/` → `ui/dragdrop/`** — `ItemDragData` is a client-domain
  drag payload (serializes `scada::NodeId`); it was never part of the aui
  targets anyway (built as `client_ui_dragdrop_*` from `base/CMakeLists.txt`).
  Its `common/node_state.h` include was narrowed to `scada/node_id.h`.
- **`aui/qt/client_utils_qt.*` → `ui/qt/client_utils_qt.*`** — `LoadPixmap`
  maps `resources/common_resources.h` command ids to `res/client.qrc` icon
  paths, both client-app resources. Now compiled into `client_ui_common_qt`.
  Its qrc-coverage unit test moved with it into `client_qt_unittests`
  (which now compiles `res/client.qrc` via AUTORCC), replacing the qrc hack
  `aui/CMakeLists.txt` used to apply to `aui_qt_unittests`.
- **`graph_qt` / `view_manager_qt` declared** — `aui_qt` now PUBLIC-links
  both, so `aui/qt/graph.h` and `aui/view_manager.h` compile with aui's own
  flags. (The C++20 facade keeps them include-only by choice: the names they
  surface belong to those libraries, not aui — see `scada_client_aui.cppm`.)
- **Link set tightened** — `scada_core` and `transport` dropped; aui includes
  nothing from them. The `scada.client.aui` facade now `export import`s
  `scada.base` only, and the module smoke test no longer references
  `scada::NodeId`.

## Remaining steps for the actual repo split

Mechanical, in whatever order the split lands:

1. **Build machinery.** The new repo needs `client_module.cmake` (the dual
   `_qt`/`_wt` target helper), which itself builds on `scada_module()` from
   scada-core, plus a definition of `CLIENT_UI_CONFIGS` (today set by the
   client root `CMakeLists.txt` from `BUILD_CLIENT_QT`/`BUILD_CLIENT_WT`).
   Either move `client_module.cmake` into the aui repo and have the client
   consume it from there, or give both repos a shared copy.
2. **Find module.** Add `FindScadaClientAui.cmake` following the existing
   pattern (`FindScadaClient.cmake`, `FindGraphQt.cmake`: guarded
   `add_subdirectory`), and extend consumers' `CMAKE_MODULE_PATH` seeds
   (top-level and client `CMakePresets.json` / `CMakeUserPresets.json`,
   client CI's `-DCMAKE_MODULE_PATH`).
3. **CI.** Add the new repo to the client workflow's dependency checkouts
   (`.github/workflows/cmake-multi-platform.yml`) next to scada-core,
   graph-qt, etc., and give the aui repo its own CI (its unit tests are
   self-contained: `aui_qt_unittests` needs no client resources anymore).
4. **Explicit test references.** `app/qt/CMakeLists.txt` compiles
   `aui/test/qt/table_unittest.cpp` / `tree_unittest.cpp` into
   `client_qt_unittests` by relative path; after the split those belong to
   the aui repo's own test target.
5. **Facade wiring.** `scada_add_module_facade` comes from scada-core, so
   `scada_client_aui.cppm` moves as-is; only the client's
   `docs/cxx-modules.md` module map needs a pointer to the new home.
6. **Namespace migration.** `aui/aui_ns_compat.h` (transitional
   `aui` → `scada::aui` using-directive) moves with the repo; finishing that
   requalification is independent of the split.

## Verification record (macOS, 2026-07-12)

Built clean under `macos-local-client`: `aui_qt`, `client_ui_common_qt`,
`client_ui_dragdrop_qt`, `client_main_window_qt`, `client_graph_qt`,
`client_qt`, `client_qt_unittests`, `client_base_unittests`,
`client_screenshot_generator`; under `macos-local-client-modules`:
`aui_qt_module_unittests`. Tests: aui suite 28/29 (the one failure,
`ThemeQtTest.ApplyThemeInstallsPaletteAndStyle`, reproduces identically with
the pre-change `aui/CMakeLists.txt` — pre-existing on this platform, tracked
separately), `ClientUtilsQtTest` 3/3 in its new home (qrc resolution
verified), main-window suite 44/44, facade smoke test 2/2. The Wt flavor has
no targets on macOS; Windows/CI covers `aui_wt` (the wt-side changes are
include-path normalizations only).
