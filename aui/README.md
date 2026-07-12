# SCADA Client AUI

The abstract UI layer of the Telecontrol SCADA client: platform-agnostic
models (tables, trees, grids, menus, properties) plus thin Qt and Wt view
wrappers, built as the dual `aui_qt` / `aui_wt` targets.

This directory works both as a subdirectory of the scada-client build
(consumed through `FindScadaClientAui.cmake`) and as a standalone top-level
CMake project, in preparation for extraction into its own repository.

## Dependencies

- **scada-core** — `scada_base` (base utilities) and the `scada_module()`
  CMake machinery. Found via `FindScadaCore.cmake` on `CMAKE_MODULE_PATH`.
- **graph_qt**, **view_manager_qt** — standalone view component libraries
  wrapped by `aui/qt/graph.h` and `aui/view_manager.h` (Qt flavor only).
- **Qt 6** (`aui_qt`) and/or **Wt** (`aui_wt`).

Nothing else: aui must stay free of scada-client and scada-common couplings.
See the client repo's `docs/aui-extraction.md` for the dependency contract.

This directory also owns `client_module.cmake` — the helper that creates the
dual `<name>_qt` / `<name>_wt` targets — which the scada-client build reuses.

## Building standalone

The checkout directory must be named `aui` (headers are included as
`"aui/<header>"` and the targets export the parent directory, the same
pattern as `graph_qt`).

```sh
cmake --preset ninja \
  -DCMAKE_MODULE_PATH="<scada-core>;<net>;<graph_qt>;<view_manager_qt>"
cmake --build --preset ninja-release
ctest --preset ninja-release
```

`BUILD_CLIENT_QT` / `BUILD_CLIENT_WT` (both default `ON`) select the flavors;
inside a consuming project the `CLIENT_UI_CONFIGS` variable set by that
project wins.

## Tests

- `aui_qt_unittests` — unit tests discovered from `*_unittest.cpp`.
- `aui_test_qt_unittests` — widget tests (`test/qt/`) plus the shared
  `AppEnvironment` test helper consumed by client modules.
- `module_test/` — C++20 module facade smoke test (`SCADA_CXX_MODULES=ON`).
