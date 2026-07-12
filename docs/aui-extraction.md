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

## Split preparation — implemented (2026-07)

Everything below is done; aui builds both in-tree and as a standalone
top-level project:

1. **Build machinery.** `client_module.cmake` (the dual `_qt`/`_wt` target
   helper) moved into `aui/`; the client reuses it from there via the find
   module. `aui/CMakeLists.txt` is top-level-capable: standalone it defines
   the project, derives `CLIENT_UI_CONFIGS` from
   `BUILD_CLIENT_QT`/`BUILD_CLIENT_WT`, and pulls `scada_module()` +
   `scada_base` via `find_package(ScadaCore)`; inside the client build the
   consumer's `CLIENT_UI_CONFIGS` and already-present machinery win.
2. **Find module.** `aui/FindScadaClientAui.cmake` (guarded
   `add_subdirectory`, same pattern as `FindGraphQt.cmake`). The client root
   no longer does `add_subdirectory(aui)` — it appends the in-tree `aui/`
   directory to `CMAKE_MODULE_PATH` as the pre-split fallback and calls
   `find_package(ScadaClientAui REQUIRED)`; a sibling checkout seeded
   earlier on `CMAKE_MODULE_PATH` takes precedence.
3. **Include prefix.** aui targets export their parent directory
   (`client_module_include_directories(aui PUBLIC "..")`, the `graph_qt`
   pattern), so `"aui/<header>"` includes work without the client root's
   `include_directories(".")`. The checkout directory must be named `aui`.
4. **Test ownership.** The duplicated `aui/test/qt/{table,tree}_unittest.cpp`
   references were removed from `client_qt_unittests`; those tests run in
   aui's own `aui_test_qt_unittests` (and `aui_qt_unittests` needs no client
   resources anymore).
5. **Repo scaffolding** (inert until the split): `aui/vcpkg.json`,
   `aui/CMakePresets.json` (`ninja` preset), `aui/.github/workflows/`
   (mirrors the client workflow), `aui/README.md`, `aui/LICENSE`.

## Remaining: the flip itself

1. **Create the repo.** Seed a new repository from `client/aui/` history
   (`git filter-repo --subdirectory-filter aui` or a fresh import) at the
   canonical remote location (the superproject's submodule URLs are local
   paths, e.g. `d:/tc/git/...` — the user creates this). The checkout/
   submodule directory must be named `aui`.
2. **Superproject.** Add the new repo as a submodule; remove `client/aui/`
   from the client repo; drop the client root's in-tree
   `CMAKE_MODULE_PATH` fallback append; seed the aui location in the
   superproject presets' `CMAKE_MODULE_PATH` (like
   `third_party/graph_qt` / `third_party/view_manager_qt` today).
3. **Client CI.** Add the aui checkout (path `aui`) to
   `.github/workflows/cmake-multi-platform.yml` and its path to the
   `-DCMAKE_MODULE_PATH` list.
4. **aui CI.** The bundled workflow activates once the repo exists; fix the
   `view_manager_qt` checkout remote in it first (that repo has no
   published remote yet).
5. **Namespace migration** (independent of the split):
   `aui/aui_ns_compat.h` — the transitional `aui` → `scada::aui`
   using-directive — moves with the repo; requalification finishes on its
   own schedule.

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

Split-preparation round (macOS, 2026-07-12, later the same day): superproject
reconfigured with the `find_package(ScadaClientAui)` consumption and rebuilt
clean on both presets (aui suite 29/29 — the ThemeQt failure was fixed
separately in f07a1429 — widget tests 9/9, ClientUtilsQt 3/3, main-window
44/44, facade smoke 2/2; `client_qt_unittests` confirmed to no longer contain
the aui table/tree tests). Standalone proof: `client/aui` configured and
built as its own top-level project (Ninja Multi-Config, scada-core +
third_party find modules on `CMAKE_MODULE_PATH`, vcpkg-installed deps +
homebrew Qt, `BUILD_CLIENT_WT=OFF`) — `aui_qt` links, `aui_qt_unittests`
29/29, `aui_test_qt_unittests` 9/9.
