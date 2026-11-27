# CLAUDE.md — Telecontrol SCADA Client

The long form — the reasoning and history behind each rule, under the same headings — is in [`CLAUDE.reference.md`](CLAUDE.reference.md).

## Project Overview

Telecontrol SCADA Client is a C++ industrial monitoring and control application (version 2.6.11): remote device monitoring, real-time and historical data, event/alarm journaling, and device configuration management. It supports SCADA/Telecontrol, OPC UA, Vidicon and Modus, and ships a Qt desktop frontend. The web client is a separate app in the superproject, not a second backend of this one.

Licensed under GPL-3.0 (see `LICENSE`).

## Repository Structure

```
scada-client/
├── app/                    # Application entry point (qt/ subdir)
│   ├── qt/                 # Qt desktop application main()
│   ├── client_application.h/.cpp  # Core application orchestrator
│   └── ...
├── aui/                    # Abstract UI layer (platform-agnostic models)
│   ├── models/             # Grid, tree, table data models
│   ├── qt/                 # Qt-specific UI implementations
│   └── test/
├── base/                   # Foundation utilities (command line, blinker, JSON, filesystem)
├── clipboard/              # Clipboard and node serialization
├── modules/                # 45 feature modules: events, graph, table, watch, write,
│                           #   configuration, favorites, filesystem, login, print,
│                           #   settings, modus + vidicon (Qt/Windows only), ...
├── controller/             # MVC controller layer, view management, command registry
│   └── test/
├── core/                   # Core module: command registries, tracer, progress host
├── main_window/            # Main window management and page/view lifecycle
├── profile/                # User profiles, window definitions, page layouts
├── properties/             # Property management and dialogs
├── services/               # Shared services (speech, task management, telemetry)
├── timed_data/             # Time-series data service
├── web/                    # Web component
├── res/                    # Resources and settings
├── test/                   # Integration tests and display tester
├── screenshots/            # Doc screenshot gallery + image_manifest.json
├── .github/workflows/      # CI: ci.yml (cppcheck + build matrix; see CI/CD below)
├── .cppcheck-suppressions  # This product's own cppcheck suppressions
├── CMakeLists.txt          # Root CMake build file
├── aui/client_module.cmake # Custom CMake helpers for `_qt` target creation (aui-owned)
├── app/qt/client_ru.ts     # The whole Russian catalog; the only .ts that ships
├── app/client_icon.rc      # Windows resource script: the app icon, nothing else
├── resources/              # Command ids (common_resources.h) + icon-strip paths
└── client_utils.cpp/.h     # Global utility functions
```

**The client backlog is not in this repo.** It is the superproject's one
`tasks.md` at `/scada/tasks.md`, **Client** section. Do not re-create
`client/tasks.md` <!-- doc-citations: absent -->.

## Documentation

**The client's design docs live in the superproject, not in this repo.** Only
`screenshots/` is repo-owned. Paths below are relative to the superproject root
and resolve only in the sibling checkout under `/scada`.

- `docs/client/design.md` — layered component architecture
- `docs/client/requirements.md` — use cases, functional and non-functional
  requirements
- `docs/client/message-loop.md` — `MessageLoopQt`, the asio I/O thread behind
  `AnyExecutor`, App Nap. Read it before changing scheduling, `MessageLoopQt`,
  or executors.
- `docs/client/ux/` — the UX design system (see "UX design system" below)
- `docs/client/command-line.md` — command-line switch documentation
- `docs/client/aui-extraction.md`, `docs/client/telemetry-gaps.md`,
  `docs/client/opcua-client-interop.md`, `docs/client/chromium-deps.md`
- `docs/ops/client-build.md`, `docs/ops/client-cxx-modules.md`,
  `docs/ops/client-screenshots.md`, `docs/ops/e2e-client-server.md`
- `docs/diagrams/client-*.puml` / `.svg` — the client architecture diagrams
- `docs/product/ui-mockups/` — the shared Qt ⇄ web screen mockups

Treat `design.md` and `requirements.md` as living documents, not snapshots.

### Doc screenshots and the web manual

The user manual (scada-docs, a sibling Jekyll repo) takes its UI images from
`client/tools/screenshot_generator/`, rendered offline from
`screenshot_data.json` — **build artifacts, not hand captures**. Workflow:
[`docs/ops/client-screenshots.md`](../docs/ops/client-screenshots.md). Source of
truth per image: [`screenshots/image_manifest.json`](screenshots/image_manifest.json).

- **Manifest first.** Every image added, retagged or removed updates its
  manifest row in the same change ("Adding a new auto-screenshot" in
  `docs/ops/client-screenshots.md`); hand captures get a `manual-*` row.
- **The gallery is tracked, publishing is gated.** `screenshots/` (PNGs
  included) is committed. It is legitimately mixed-platform: **a changed image
  whose `captured.platform` also changed is churn; same platform means a UI
  change.** Publish in two steps (the second from the superproject root),
  **always passing `SCADA_SCREENSHOT_GENERATOR`** so a stale PNG is refused,
  then review `git diff` on the manual's `img/`. Only reviewed images join
  `current_generator_owned_subset`.

  ```bash
  cmake --build --preset relwithdebinfo -t regenerate_client_screenshots
  cmake -DSCADA_SCREENSHOT_SRC_DIR=client/screenshots \
        -DSCADA_DOCS_IMG_DIR=scada-docs/img \
        -DSCADA_SCREENSHOT_GENERATOR=<path to the generator binary> \
        -P cmake/update_screenshots.cmake
  ```
- **The gallery is TWO passes, one per appearance** — re-render both, and
  stamp both (`--stamp --produced-since <marker>`); check pairs with
  `python3 tools/screenshot_generator/capture_provenance.py --check-pairs`.

  ```bash
  client_screenshot_generator --out client/screenshots
  client_screenshot_generator --out client/screenshots --theme light
  ```
- **Validate consistency** after touching images, the manifest or manual pages:
  `python3 screenshots/validate_image_manifest.py` (or pass `--docs-repo`).
- **When UI changes, regenerate**, refresh published copies in the same effort,
  and retag orphaned images `obsolete`.
- **New user-visible functionality gets BOTH a screenshot and a manual
  description in the same effort** — the scada-docs page (Russian + `en/`
  mirror + `_data/i18n_pages.yml`) and the row's `referenced_from`;
  workbench-shell features go on `client/workbench.md`.
- **macOS may render the published gallery, Windows is needed only for Modus**
  — only the hand-maintained `manual-modus` family. Two names mislead:
  `substation-display.png` (VDS renderer) and `menu-scheme.png` (a `QMenu`) do
  not use ActiveX and may be rendered on macOS.
- **The unit tests run offscreen on macOS too, by default**, through
  `AppEnvironment` (`aui/test/qt/app_environment.h`). A test binary that builds
  its own `QApplication` instead steals focus per case — take the fixture. Name
  your own `offscreen:configfile=` for a non-1920x1080 screen; on Windows a bare
  `offscreen` is left alone. See `docs/ops/client-build.md`.

### UX design system

The client's UX design system lives under [`docs/client/ux/`](../docs/client/ux/README.md).
**Read it before adding or restyling any UI**, and follow it rather than
inventing chrome. **The direction is a native desktop look and feel:**

- **Use the platform Qt style**; never `QApplication::setStyle("Fusion")`.
- **Colour through `QPalette` roles**, not stylesheets — a new baked-colour
  `setStyleSheet` is a regression.
- **Size through `QStyle::PixelMetric`, `QFontMetrics`, `QApplication::font()`**
  — never hard-coded px sizes, widths or radii.
- **Follow the OS light/dark preference by default**, with an override applied
  live via `QEvent::ApplicationPaletteChange`.
- **Prefer stock Qt widgets in their conventional roles** and native dialogs.
- **Dialogs follow [`docs/client/ux/dialogs.md`](../docs/client/ux/dialogs.md)**: no repeated title
  or brand mark in the content; always `QDialogButtonBox`.
- **Exception — process semantics are ours, not the platform's.** Alarm
  severity, data quality and equipment state are safety colours (ISA-101,
  ISA-18.2/EEMUA 191): their meaning, hue family and order are fixed in every
  scheme and only their lightness is tuned per scheme, they never follow the
  accent, and they are always paired with a label or shape.

The information architecture (Activity bar → Explorer → workspace tabs →
Inspector → status strip) stands; only the chrome is native. The mockups in
[`docs/product/ui-mockups/screens/`](../docs/product/ui-mockups/screens/) are
**the north star for information architecture**, **not a visual target** —
validate implemented UI against real Qt widgets via `client_screenshot_generator`
(see [`docs/client/ux/README.md`](../docs/client/ux/README.md) → Mockups). The
cross-client obligation is in the superproject `CLAUDE.md`.

- [`docs/client/ux/principles.md`](../docs/client/ux/principles.md) — HMI/SCADA UX principles; the *why*.
- [`docs/client/ux/design-language.md`](../docs/client/ux/design-language.md) — tokens and primitives; never hard-code hex.
- [`docs/client/ux/shell.md`](../docs/client/ux/shell.md) — the shell mapped onto `main_window/` / `modules/`.
- [`docs/client/ux/backlog.md`](../docs/client/ux/backlog.md) — surface catalogue; a **menu of slices, not a fixed waterfall**.

### UX implementation approach

**Do not follow `backlog.md` as a strict P0→P5 waterfall.**

- **Incremental vertical slices.** Ship one coherent surface end-to-end at a
  time and re-evaluate; no big-bang switchover.
- **Full reshell is the north star, reached gradually** — slices converge on
  the operator-workbench structure.
- **Theming is palette-first, and no longer opt-in.**
  `scada::aui::ApplyTheme` ([`aui/qt/theme_qt.h`](aui/qt/theme_qt.h)) is the
  only appearance. `InstalledAppearance` (`app/qt/installed_appearance.h`)
  applies it at startup from the `Ux/Theme` QSetting (`system` by default); the
  operator changes it in **Settings → Colour scheme** (`AppearanceMenuModel` in
  `main_window/main_menu/`).
  - There is no "no theme" state (`SeverityTheme::kLegacy`, `ClearTheme()`,
    `IsThemeInstalled()` and `Ux/Experimental` are gone). Do not reintroduce
    one: a widget decides how to draw, its host decides whether to build it.
  - `ApplyTheme` owns the severity ramp — never call `SetSeverityTheme`
    alongside it.
  - Prefer recolouring through `QPalette` (`ThemeScope::kPaletteOnly`). The
    global stylesheet (`kFull`) is a single rule; `BuildThemeStyleSheet` records
    what was removed and a unit test fails if it returns. Never grow the global
    sheet, and never make it unconditional.
- **Every removed `setStyleSheet` is progress.** Adding one needs a reason
  `QPalette` plus `QStyle::PixelMetric` could not serve, stated in a comment.
- **Validate by purpose, and validate in both OS appearances.** Mockups for
  information architecture only; implemented UI against **real Qt widgets** via
  `client_screenshot_generator` (see `docs/ops/client-screenshots.md`). A
  native-look change is not done until seen under both a light and a dark
  system theme.

When in doubt about scope or sequence, ask rather than executing the backlog
top to bottom.

### When to update the docs

**Update `docs/client/design.md`, `docs/client/requirements.md`, and the relevant diagram
whenever you change or add functionality.** At minimum:

- Adding or removing a top-level module (`*_module.{h,cpp}`) — update the
  module table and `docs/diagrams/client-module-graph.puml`.
- Adding or removing a directory under `client/` that hosts a new layer or
  domain area — update the layer description and
  `docs/diagrams/client-architecture-layers.puml`.
- Adding or removing a back-end registered with `REGISTER_DATA_SERVICES` —
  update FR-1 in `docs/client/requirements.md` §3.
- Changing the bootstrap order in `ClientApplication::PostLogin()` —
  update `docs/diagrams/client-bootstrap-sequence.puml`.
- A new actor-facing capability not covered by an existing use case — add a
  row to the use-case table in `docs/client/requirements.md` §2 and an FR in §3.
- Removing a use case — strike the row in §2 and the matching FR.
- Changing a design token, component primitive, or a shell region — update the
  matching `docs/client/ux/` doc **and** the affected mockup in
  `docs/product/ui-mockups/screens/` in the same change, then regenerate the touched
  `screenshots/` image once the code lands.

If you cannot tell whether a change affects the doc, ask. Drift between the doc
and the code is worse than no doc.

### Diagrams

Client diagrams live in the superproject's `docs/diagrams/`, prefixed
`client-`. Paths are relative to the superproject root.

| File | Renders to | Used in |
|---|---|---|
| `docs/diagrams/client-use-cases.puml` | `client-use-cases.svg` | `docs/client/requirements.md` §2 (use cases) |
| `docs/diagrams/client-architecture-layers.puml` | `client-architecture-layers.svg` | `docs/client/design.md` §3 (component overview) |
| `docs/diagrams/client-module-graph.puml` | `client-module-graph.svg` | `docs/client/design.md` §3.6 (domain modules) |
| `docs/diagrams/client-bootstrap-sequence.puml` | `client-bootstrap-sequence.svg` | `docs/client/design.md` §3.1 (startup sequence) |
| `docs/diagrams/client-opcua-discovery-flow.puml` | `client-opcua-discovery-flow.svg` | `docs/client/opcua-client-interop.md` |

**To update a diagram:** edit the `.puml`, regenerate, **look at a rendered
PNG** before committing, and commit `.puml` and `.svg` together.

```bash
plantuml -tsvg docs/diagrams/client-<name>.puml
plantuml -tpng docs/diagrams/client-<name>.puml -o /tmp   # to look at
```

Start every diagram with `!include _style.puml` (house style: superproject
`CLAUDE.md`, "PlantUML house style"); colour with `:text; <<$tier>>` after the
semicolon; no `<style>` stereotype blocks; no trailing `' comment` on a
`!$var` line; end with a newline. A new diagram is referenced from
`docs/client/design.md` as `![alt](../diagrams/client-<name>.svg)` with a
"Source: …" caption, and added to the table above.

## Build System

### CMake (Primary — Cross-Platform)

Hierarchical CMake; each module directory has its own `CMakeLists.txt`.

**CMake Presets:**

`CMakePresets.json` is self-contained and carries the same preset names as every other product in the tree (ADR 0011). Machine-specific settings go in `.scada-local.cmake` beside `build-support/`, never in a per-repo `CMakeUserPresets.json`.

| Type | Preset | Description |
| ---- | ------ | ----------- |
| Configure | `ninja` | Ninja Multi-Config, vcpkg toolchain |
| Build | `debug` / `release` / `relwithdebinfo` | |
| Test | `test-debug` / `test-release` | |

```bash
cmake --preset ninja                        # Configure (once)
cmake --build --preset release              # Build (Release)
cmake --build --preset relwithdebinfo       # Build (RelWithDebInfo)
cmake --build --preset debug                # Build (Debug)
ctest --preset test-release                 # Test (Release)
ctest --preset test-debug                   # Test (Debug)
```

### Custom CMake Module System

`aui/client_module.cmake` (owned by aui; the client gets it via `find_package(ScadaClientAui)`) creates one Qt target per module, `<name>_qt`:

- `client_module(name)` — Creates the Qt library target
- `client_module_sources(name PUBLIC|PRIVATE dirs...)` — Adds sources from directories (auto-includes the `dir/qt/` subdir)
- `client_module_link_libraries(name PUBLIC|PRIVATE libs...)` — Links libraries, auto-resolving `_qt` suffixed targets
- `client_module_include_directories(name PUBLIC|PRIVATE dirs...)` — Adds include directories

Qt targets get `AUTOMOC`, `AUTOUIC`, `AUTORCC` and `.ts` processing automatically.

### Key Dependencies

Managed via `vcpkg.json` manifest:

- **Qt 6** — `qtbase` (Widgets, PrintSupport), `qttools` (LinguistTools), `qtactiveqt` (Windows)
- **Boost** — `boost-asio`, `boost-beast`, `boost-signals2`, `boost-locale`, `boost-range`, `boost-algorithm`
- **Google Test** — `gtest`

Not managed by vcpkg:

- **OPC UA SDK** — the `opcuapp` product at `third_party/opcuapp`, resolved by
  name through `scada_find_products`, not by path
- **Modus 6.30** — ActiveX/COM library (Windows/Qt only, via `${deps}/modus`)
- **Windows SDK / ATL** — COM/ActiveX support (Windows only)

## CMake Options

| Option | Default | Description |
| ------ | ------- | ----------- |
| `BUILD_OPC` | `ON` | Build Classic OPC modules in scada-common (Windows only) |
| `BUILD_VIDICON` | `ON` | Build Vidicon modules in scada-common (Windows only) |

The `vidicon` client module is skipped when `scada_common_opc` and `scada_common_vidicon` targets are not available.

## C++20 Module Facades (SCADA_CXX_MODULES)

`-DSCADA_CXX_MODULES=ON` (default OFF; build unchanged when OFF) adds
named-module facades (`scada.client.base`, `scada.client.aui`,
`scada.client.controller`, ...), compiled against the `_qt` targets' flags so
`UI_QT` is defined in every BMI. See `docs/ops/client-cxx-modules.md` and
`core/docs/cxx-modules.md`.

## CI/CD

`.github/workflows/ci.yml` runs on the published export
(`github.com/alexsmn/scada-client`) on push/PR to `main` and `release/**`, and on
`v*` tags. Two jobs gate: `analyze` (cppcheck) and `build`
(Windows/Ubuntu/macOS, Release).

**On a `v*` tag it also cuts a release.** The tag must equal
`project(scada-client VERSION …)`. macOS and Windows run
`tools/release/package.sh` (`.dmg`, `.zip`); dry-run it locally after a Release
build with `tools/release/package.sh build/ninja <version> <out dir>`.

- Packages go into the product release for the tag (only the notes between the
  `client-packages` markers are rewritten), or a new **draft** a person
  publishes. **Only a release carrying the Server MSI may be marked latest**;
  publish a Client-only one with
  `gh release edit <tag> --draft=false --latest=false`.
- **The display runtime is never packaged** while its licence is unsettled
  (superproject tasks.md 788); lifting that is a deliberate script edit.
- **Signing is optional; secrets switch it on, and each group is
  all-or-nothing:** `MACOS_CERTIFICATE_P12_BASE64` +
  `MACOS_CERTIFICATE_PASSWORD` (Developer ID, hardened runtime); `APPLE_ID` +
  `APPLE_TEAM_ID` + `APPLE_APP_SPECIFIC_PASSWORD` (notarisation). Windows is
  not signed. Notary status is read from JSON because `notarytool submit
  --wait` exits 0 on `Invalid`
  ([notarytool(1)](https://keith.github.io/xcode-man-pages/notarytool.1.html),
  verified 2026-11-07).
- **The `.dmg`'s bundle is not the build's bundle byte for byte:** catalogs
  move to `Contents/Resources/translations` with a symlink left behind, and
  `app/qt/client.entitlements` disables library validation for the display
  runtime.
- **The macOS minimum is whatever the runner builds for** (no
  `CMAKE_OSX_DEPLOYMENT_TARGET`).
- **Every action in this workflow is pinned by SHA**, release URL in a comment.

A client with no display runtime beside it builds and runs (ADR 0013);
schematic windows report no runtime. CI cannot run `client_server_e2e_tests`
or a Debug leg.

**The build job gates, and must not get its `continue-on-error` back.**

**The Windows leg runs the Qt tests offscreen, and needs three things to:**
`QT_PLUGIN_PATH`, `QT_QPA_FONTDIR` = `C:\Windows\Fonts`, and `AppEnvironment`
leaving a bare `offscreen` alone.

`analyze` runs `scada_configure_cppcheck()`'s configuration
(`build-support/ScadaProductBase.cmake`) with `.cppcheck-suppressions`;
`error:` findings gate. **The job builds a pinned cppcheck (`CPPCHECK_VERSION`
in the workflow, 2.21.0) from source and caches it, rather than installing the
distro package.** **When comparing counts, say which cppcheck produced them**,
and reproduce CI with the pinned version (`ubuntu-latest` is `ubuntu:24.04`).

## Architecture

### Module-Based MVC with Dependency Injection

Modular MVC with explicit context-based dependency injection.

**Bootstrap flow:**
```
main() → AppInit → ClientApplication → [CoreModule, EventModule, MainWindowModule, ...]
```

**Core pattern — Context structs for DI:**

Each module defines a `*Context` struct and privately inherits from it:

```cpp
struct EventModuleContext {
  AnyExecutor executor_;
  Profile& profile_;
  scada::services services_;
  // ...
};

class EventModule : private EventModuleContext {
 public:
  explicit EventModule(EventModuleContext&& context);
  // ...
};
```

### Key Subsystems

| Subsystem | Entry Point | Responsibility |
|-----------|-------------|----------------|
| `ClientApplication` | `app/client_application.h` | Top-level orchestrator; owns all modules |
| `CoreModule` | `core/core_module.h` | Command registries, tracer, progress host |
| `MainWindowModule` | `main_window/` | Window lifecycle, view management, page navigation |
| `EventModule` | `modules/events/event_module.h` | Event fetching, journaling, local events |
| `ControllerRegistry` | `controller/controller_registry.h` | Maps command IDs to controller factories |
| `Profile` | `profile/profile.h` | User preferences, window layouts, page definitions |
| `NodeService` | via factory | Device/node browsing and monitoring |
| `TimedDataService` | via factory | Historical and real-time time-series data |
| `TaskManager` | `services/` | Async task execution and progress |

### Data Service Backends

Three pluggable backends registered via `REGISTER_DATA_SERVICES` macro:

- **Scada** ("Telecontrol") — Default, connects to `localhost`
- **OPC UA** — Standard industrial protocol, `opc.tcp://localhost:4840`
- **Vidicon** — Custom protocol, connects to `localhost`

### Controller Registration

Controllers are registered statically via macros:

```cpp
REGISTER_CONTROLLER(MyController, my_window_info);
```

Or dynamically via `ControllerRegistry::AddControllerFactory()`.

### Platform Abstraction

Each module that has UI splits code into:
- **Shared model code** — in the module root directory, free of any widget toolkit
- **`qt/` subdirectory** — Qt-specific implementation

`aui_qt` publishes the `UI_QT` define. Keep new model code on the toolkit-free side so it stays unit-testable without a `QApplication`.

## Coding Conventions

### Naming

- **Classes/Types:** `PascalCase` — `ClientApplication`, `WindowDefinition`, `EventModule`
- **Methods:** `PascalCase` — `Start()`, `CreateTree()`, `GetControllerFactory()`
- **Member variables:** `snake_case_` with trailing underscore — `profile_loaded_`, `quit_promise_`
- **Local variables:** `snake_case` — `alias_resolver`, `audited_services`
- **Constants:** `kPascalCase` — `kTableLimitation`
- **Namespaces:** `snake_case` — `base::`, `scada::`, `net::`
- **Files:** `snake_case.cpp/.h` — `client_application.cpp`, `event_module.h`
- **Test files:** `*_unittest.cpp` — `event_module_unittest.cpp`
- **Mock files:** `*_mock.h`

### Header Guards

`#pragma once` (no `#ifndef` guards).

### Includes

Ordered as: project headers, then third-party/standard headers, separated by blank lines:

```cpp
#include "app/client_application.h"

#include "base/blinker.h"
#include "core/core_module.h"
// ... project headers

#include <memory>
#include <stack>
```

### Modern C++ (C++23)

The client builds at **C++23** (`scada_product_base()`'s default). Use smart
pointers (no raw `new`/`delete`); designated initializers for context structs
(`.field_ = value`); `std::optional<T>`; `std::string_view` for non-owning
string parameters; `std::ranges`/`std::views`; structured bindings;
`[[nodiscard]]` on functions returning promises/important values; `= delete`
for non-copyable classes; `std::function` callbacks; `std::bind_front`;
`explicit Module(Context&& context)`; `std::chrono_literals` (`1min`).

### Memory Management

- Owned resources use `std::unique_ptr`
- Shared resources use `std::shared_ptr`
- Singleton-like modules stored in `std::stack<std::shared_ptr<void>> singletons_` for ordered destruction
- Destructor resets members in dependency-safe order

### Error Handling

- Async operations use coroutine bodies; legacy/public boundaries may still
  return `promise<T>` from `scada-core`.
- Exceptions for fatal errors (`std::runtime_error`)
- Local event system (`LocalEvents`) for user-visible errors/warnings

### Concurrency

- `boost::asio::io_context` for async I/O
- `Executor` abstraction for task scheduling
- Public and module-crossing async APIs may keep returning `promise<T>`, but
  client implementation code is coroutine-first.
- Use `AwaitPromise(...)` and `ToPromise(...)` only at legacy boundaries
  instead of adding `.then()` chains.

### Localization

- UI strings use `u"..."` (UTF-16 literals) for Russian text; `.ts` files live
  in `qt/` subdirectories, named `*_ru.ts`.
- **Never run `lupdate` against `app/qt/client_ru.ts`** — it does not recognise
  `Translate()`, so most strings would go `vanished` and stop shipping.
  Maintain it by hand; `lupdate` a single form into a scratch `.ts` and merge.
- **Never hard-code a user-facing string as a `u"..."` literal.**
  `client_untranslated_string_check` (ctest,
  `tools/check_untranslated_ui_strings.py`) fails on one reaching a message
  box, `ResourceError`, file-dialog title or `setWindowTitle`. A
  namespace-scope `const char16_t k…[]` cannot call `Translate()` — make it a
  function returning `std::u16string`.
- **Never write Russian in a string literal** (same check; comments are fine;
  `modules/modus/activex/` is allowlisted). Pick an English source not already
  in `client_ru.ts`'s empty context with a different translation, and keep the
  displayed Russian unchanged.
- **The same rule now covers `core/` and `common/`, which is where the rest of
  the operator's text comes from.** They carry invariant forms only
  (`Bad_Timeout`, `STALE`); the words are in `services/core_ui_text.cpp`
  (installed by `InstallCoreUiText()` in `AppInit`), Russian in `client_ru.ts`.
  A new status code needs a row there and a catalog entry
  (`CoreUiTextTest.EveryBadCodeHasASentence`).
- **A `.ui` form's strings belong to the form class's context**, not the empty
  one. `client_ui_translation_check` (ctest, `tools/check_ui_translations.py`)
  enforces the catalog rules below.
- **That check is also the catalog-completeness check, and it compares keys,
  not files.** Rules 1 and 6 need `lupdate` on `PATH`; without it ctest reports
  **Skipped**, meaning they did not run.
- **The same check has two more rules, and both run without `lupdate`:** no two
  shipping messages in one context may share a source (`lrelease` drops all
  but one); and every `Translate("literal")` must have a shipping translation
  (`TRANSLATE_GAPS` is empty — keep it so).
- **Design-time placeholder text in a `.ui` takes `notr="true"`, not a parked
  entry** in `UI_FORM_ALLOWED_UNTRANSLATED`.
- **Every message in the catalog must be one `lrelease` compiles** (rule 10):
  no `vanished`/`obsolete`/`unfinished`/empty entries, no parked list.
- **A compiled catalog must be copied by a rule that depends on the `.qm`, not
  on the link** (rule 9): in `app/qt/CMakeLists.txt` use
  `add_custom_command(OUTPUT ...)` + a target + `add_dependencies()`, never
  `POST_BUILD`, and spell the bundle path out (no
  `$<TARGET_BUNDLE_CONTENT_DIR:client_qt>` in an OUTPUT). Inside a macOS bundle
  `InstalledTranslation` reads only `Contents/MacOS/translations`.
- **Two further rules catch the shapes where the string never appears at the
  call site** — a table (`Translate(column.header)`, rule 7) and a function
  (`Translate(WriteBlockText(block))`, rule 8). Declare such members/returns
  `const char*` or `std::string_view` and add every string to `client_ru.ts`;
  what the rules cannot resolve is reported, not skipped.

## Testing

### Test Framework

Unit tests are `*_unittest.cpp`, alongside the code they test, run with CTest.

### Test Locations (examples)

- `app/client_application_unittest.cpp`
- `modules/table/table_model_unittest.cpp`
- `modules/events/event_table_model_unittest.cpp`
- `main_window/main_window_unittest.cpp`
- `profile/page_layout_unittest.cpp`

### Reusable Test Helpers

Put reusable test helpers — shared fakes, fixture bases, mock-factory
functions, test-only utilities used by more than one module — under
`client/test/`. When a helper starts getting copy-pasted across modules, move
it there and link the module's test target against it.

### Running Tests

```bash
ctest --preset test-release                 # Release
ctest --preset test-debug                   # Debug
```

## Command-Line Switches

Logging-related switches (pass as `--switch-name`):

- `verbose-logging`
- `log-service-read`
- `log-service-browse`
- `log-service-history`
- `log-service-event`
- `log-service-model-change-event`
- `log-service-node-semantics-change-event`

## Key Patterns for AI Assistants

1. **New modules** follow the Context + private inheritance pattern: a `*Context` struct, privately inherited, `Context&&` in the constructor.
2. **New UI components** keep toolkit-specific code in a `qt/` subdirectory; shared logic in the module root. Use `client_module()` in CMake.
3. **New controllers** are registered via `REGISTER_CONTROLLER(ControllerClass, window_info)` or through `ControllerRegistry`.
4. **Tests** go alongside source as `*_unittest.cpp` files.
5. **Member variables** always end with a trailing underscore.
6. **Headers** always use `#pragma once`.
7. **Dependencies** between modules are explicit through Context struct fields — avoid hidden globals.
8. **Destruction order matters** — `ClientApplication::~ClientApplication()` resets members in dependency order; follow it when adding modules.
9. **Conditional compilation** — `#if defined(UI_QT)` for code needing the Qt UI config; `#if defined(_WIN32)` for Windows-only code such as the Modus/Vidicon ActiveX paths.
10. **Async code** keeps `promise<T>` at public/module boundaries; new or touched implementation code uses coroutine bodies with `co_await`. `AwaitPromise(...)` awaits legacy promises, `ToPromise(...)` only at compatibility boundaries; no new `.then()` chains.
11. **Modus/Vidicon ActiveX parameter names** — Never rename OLESTR parameter names in `modules/modus/` (e.g., `"ключ_привязки"`, `"положение"`, `"уставки"`); they are the external Vidicon ActiveX protocol interface.
12. **Add a regression unit test for every fixed bug** — one that fails against the pre-fix code. If it would need a major redesign (new mock layer, restructuring the class, splitting a module), confirm scope with the user first; prefer a targeted test on existing seams.
13. **`aui/` is slated for extraction into its own repository.** Never add
    includes of client-repo headers (`profile/`, `resources/`, `ui/`,
    `modules/`, `main_window/`, …) or scada-common dependencies inside
    `aui/` — invert the dependency instead. Allowed dependencies:
    `scada_base`, `graph_qt`, `view_manager_qt`, Qt — see
    `docs/client/aui-extraction.md`.
