# C++20 Modules: the client/ facade modules

Status: experimental, opt-in via `-DSCADA_CXX_MODULES=ON` (default OFF).
When OFF, the build is unchanged — no module targets exist, nothing scans
for imports, PCH stays enabled.

This extends the core/common facade set to the client repo. The design,
consumer rules, toolchain matrix, and gotchas live in
`core/docs/cxx-modules.md` — read that first. This file documents only the
client-specific module map and exclusions.

## Module map

| Module | Facade target | Interface unit | `export import`s |
|---|---|---|---|
| `scada.client.base` | `client_base_module` | `base/scada_client_base.cppm` | `scada.base` |
| `scada.client.core` | `client_core_module` | `core/scada_client_core.cppm` | `scada.base` |
| `scada.client.clipboard` | `client_clipboard_module` | `clipboard/scada_client_clipboard.cppm` | `scada.node_service` |
| `scada.client.aui` | `aui_qt_module` | `aui/scada_client_aui.cppm` | `scada.base`, `scada.core` |
| `scada.client.profile` | `client_profile_qt_module` | `profile/scada_client_profile.cppm` | `scada.client.aui` |
| `scada.client.controller` | `client_controller_qt_module` | `controller/scada_client_controller.cppm` | `scada.client.aui`, `scada.client.base`, `scada.client.profile`, `scada.node_service`, `scada.timed_data` |
| `scada.client.services` | `client_services_qt_module` | `services/scada_client_services.cppm` | `scada.client.aui`, `scada.common` |
| `scada.client.properties` | `client_properties_qt_module` | `properties/scada_client_properties.cppm` | `scada.client.aui`, `scada.node_service` |

Each facade `export import`s the facades of its library's PUBLIC-linked
dependencies (the module analogue of PUBLIC link transitivity). PUBLIC links
without a facade (`transport`) stay textual for consumers.

## The facade set is Qt-flavored

`client_module()` libraries build twin `_qt`/`_wt` targets from the same
root headers, differing in the `UI_QT`/`UI_WT` PUBLIC defines. A BMI is
compiled against exactly one define set, so only the **Qt** flavor is
facaded (`aui_qt`, `client_profile_qt`, ...); the wt flavor stays
header-based. Even the flavor-neutral `client_core` facade is compiled with
`UI_QT` (see `core/CMakeLists.txt`): `node_command_context.h` pulls
`aui/key_codes.h`, whose contents exist only under a UI config. Wt TUs must
keep including headers textually; that is ODR-safe alongside Qt-side
imports because every entity stays attached to the global module.

## Not facaded in client

- `client_web` — INTERFACE library, no compiled surface.
- `client_ui_common_qt` / `client_ui_dragdrop_qt` / `client_ui_progress_qt`
  (+ `_wt` twins) — glue libraries assembled from other directories'
  sources.
- `main_window`, `app`, `modules/*` (the ~22 `client_component_*` UI
  components and domain modules), `test/`, `tools/` — application-assembly
  and leaf-feature targets, the client analogue of the `node_service_v*`
  implementation-target exclusion in common.
- `client_properties_transport` — PRIVATE-linked implementation detail of
  `client_properties`.
- Windows-only surfaces (Modus, Vidicon integration) — unbuildable on the
  macOS iteration platform.

## Client-specific header exclusions

Documented per facade in each `.cppm` header comment. Highlights:

- `base/memory_istream.h` — unguarded Windows-only content AND its
  `::MemoryIStream` collides with the identical class scada.base exports
  from `core/base/memory_istream.h` (the common-repo
  `session_proxy_notifier.h` precedent).
- `base/client_paths.h` `DIR_*`/`PATH_*` keys — enumerators of an unnamed
  namespace-scope enum (no linkage, unexportable); the header stays
  include-only for them.
- `services/sapi.h`, `services/atl_module.h` — Windows COM/ATL surfaces.
- `aui/color_win.h`, `aui/rect_internal.h`, all `*_mock.h`/`*_fake.h` —
  Windows-only / internal / test-only.
- `aui/graph.h`, `aui/view_manager.h` — thin wrappers over the external
  `graph_qt` / `view_manager_qt` component libraries (declared `aui_qt`
  dependencies since the aui-extraction decoupling). The names they
  surface belong to those libraries, not aui. Include-only.
- `aui/os_exchange_data.h` is in the GMF on non-Windows only: its `_WIN32`
  branch declares COM members without self-contained includes, so
  `aui::OSExchangeData` stays include-only on Windows.
- `REGISTER_CONTROLLER` and other macros are never importable — keep the
  owning header's textual `#include` where they are used.

## How to build

- macOS: `cmake --preset macos-local-client-modules`, then
  `cmake --build --preset client-macos-local-modules` (the client app) or
  `cmake --build --preset client-module-tests-macos-local` (the eight
  `*_module_unittests` smoke tests + `client_qt_unittests`).
- Windows: configure `ninja-dev-modules` (CMake >= 3.28); the facade
  targets appear once `BUILD_CLIENT_QT=ON`. MSVC validation is pending,
  same as core/common (see the checklist in `core/docs/cxx-modules.md`).

## Pilot importing TUs

- `base/file_settings_store.cpp` — imports `scada.base`
  (`SCADA_USE_BASE_MODULE`).
- `clipboard/node_serialization.cpp` — imports `scada.node_service`
  (`SCADA_USE_NODE_SERVICE_MODULE`); its protobuf-facing
  `remote/protocol_utils.h` and internal-linkage
  `scada/standard_node_ids.h` includes stay textual, per the core rules.
