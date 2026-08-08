# CLAUDE.md — Telecontrol SCADA Client

## Project Overview

Telecontrol SCADA Client is a C++ industrial monitoring and control application (version 2.6.0). It provides remote device monitoring, real-time and historical data viewing, event/alarm journaling, and device configuration management. The application supports multiple industrial protocols (SCADA/Telecontrol, OPC UA, Vidicon, Modus) and ships with dual UI frontends: Qt 5 (desktop) and Wt (web).

Licensed under Apache 2.0.

## Repository Structure

```
scada-client/
├── app/                    # Application entry points (qt/ and wt/ subdirs)
│   ├── qt/                 # Qt desktop application main()
│   ├── wt/                 # Wt web application main()
│   ├── client_application.h/.cpp  # Core application orchestrator
│   └── ...
├── aui/                    # Abstract UI layer (platform-agnostic models)
│   ├── models/             # Grid, tree, table data models
│   ├── qt/                 # Qt-specific UI implementations
│   ├── wt/                 # Wt-specific UI implementations
│   └── test/
├── base/                   # Foundation utilities (command line, blinker, JSON, filesystem)
├── clipboard/              # Clipboard and node serialization
├── modules/             # ~22 reusable UI components
│   ├── about/              # About dialog
│   ├── debugger/           # Request/response protocol debugger
│   ├── device_metrics/     # Device performance metrics
│   ├── limits/             # Value limit editor
│   ├── login/              # Authentication dialog
│   ├── sheet/              # Spreadsheet-like view
│   ├── summary/            # Data summary view
│   ├── table/              # Generic table component
│   ├── timed_data/         # Time-series data view
│   ├── watch/              # Real-time value watch
│   ├── write/              # Write values to devices
│   ├── configuration/      # Device/node configuration management
│   ├── events/             # Event system, event journal, local events
│   ├── export/             # Export functionality
│   ├── favorites/          # Bookmarks/favorites management
│   ├── filesystem/         # File system operations and caching
│   ├── graph/              # Graph/chart visualization
│   ├── modus/              # Modus 6.30 ActiveX/COM integration (Qt only)
│   ├── node_service_progress_tracker/
│   ├── opcua_services/
│   ├── portfolio/          # Portfolio management
│   ├── print/              # Print and print preview
│   ├── vidicon/            # Vidicon protocol integration (Qt only)
│   └── ...                 # (select_item, node_properties, node_table, etc.)
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
├── .github/workflows/      # CI: cmake-multi-platform.yml, msbuild.yml
├── CMakeLists.txt          # Root CMake build file
├── aui/client_module.cmake # Custom CMake helpers for dual Qt/Wt target creation (aui-owned)
├── translation.cmake       # Qt translation support
├── app/client_icon.rc      # Windows resource script: the app icon, nothing else
├── resources/              # Command ids (common_resources.h) + icon-strip paths
└── client_utils.cpp/.h     # Global utility functions
```

**The client backlog is not in this repo.** Like the design docs, it moved to
the superproject: the whole tree now shares one `tasks.md` at `/scada/tasks.md`,
and client-only work lives in its **Client** section. Do not re-create
`client/tasks.md`.

## Documentation

**The client's design docs live in the superproject, not in this repo.** Only
the generated screenshot gallery (`screenshots/`) is repo-owned; every
prose doc and diagram moved to `/scada/docs/` so the Qt and web clients share
one documentation tree. Paths below are relative to the superproject root and
resolve only in the sibling checkout under `/scada`.

- `docs/client/design.md` — layered component architecture, grounded in
  concrete source files
- `docs/client/requirements.md` — use cases, functional and non-functional
  requirements
- `docs/client/message-loop.md` — how the client schedules async work: the
  event-driven `MessageLoopQt` pump, the asio I/O thread behind `AnyExecutor`,
  and the macOS App Nap freeze that made all of it visible. Read it before
  changing anything about scheduling, `MessageLoopQt`, or executors.
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

The user-facing web manual is a **separate repo** (scada-docs, Jekyll on
GitHub Pages, usually checked out as a sibling of this repo). Almost all of
its UI images are rendered offline by
`client/tools/screenshot_generator/` from the JSON fixture
`screenshot_data.json` — they are **build artifacts, not hand captures**.
The authoritative design/workflow doc is
[`docs/ops/client-screenshots.md`](../docs/ops/client-screenshots.md); the source of truth for every
manual image (tag, referencing pages, publish subset) is
[`screenshots/image_manifest.json`](screenshots/image_manifest.json).

Rules of the pipeline:

- **Manifest first.** Every image added to, retagged in, or removed from
  the manual gets its manifest entry updated in the same change. New
  screenshots follow the "Adding a new auto-screenshot" flow in
  `docs/ops/client-screenshots.md` (fixture entry + capture spec + manifest row);
  hand-captured images still get a `manual-*` manifest row.
- **The gallery is tracked, publishing is gated.** `screenshots/` — PNGs
  included — is committed, so a UI change lands as a reviewable image diff.
  **Not yet a verified Windows baseline**, though: the images tracked at the
  outset were committed with their provenance unestablished and at least 12 are
  macOS renders, so the first Windows regeneration rewrites an unknown number
  of them as platform churn. Read `docs/ops/client-screenshots.md` before
  treating a diff here as a UI change.
  Publishing to the manual is a separate, narrower step:
  `cmake --workflow --preset update-screenshots-dev`
  (Windows) regenerates the gallery
  and copies only the manifest's `current_generator_owned_subset` into
  scada-docs `img/`; review with `git diff img/` there. An image graduates
  into that subset only after its rendering is reviewed against the page
  that embeds it. **Published images render dark** — the gallery pass is
  legacy-themed, then a second `--theme=dark` pass re-renders the published
  subset on top, so the manual reads as one product. An image that cannot
  render themed opts out with `"publish_theme": "legacy"` plus a reason, or
  the regeneration fails by name; see `docs/ops/client-screenshots.md`.
- **Validate consistency** after touching images, the manifest, or manual
  pages: `python3 screenshots/validate_image_manifest.py` (auto-finds
  a sibling scada-docs checkout, or pass `--docs-repo`).
- **When UI changes, regenerate.** A diff in the generated PNGs is the
  visual-regression signal; refresh the published copies in the same
  effort as the UI change, and retag orphaned images `obsolete` when
  removing features.
- **New user-visible functionality gets BOTH a screenshot and a manual
  description in the same effort.** Add/extend the scada-docs page (Russian
  canonical + the `en/` mirror + `_data/i18n_pages.yml`) describing the
  behaviour, embed the capture, and keep the manifest row's
  `referenced_from` in sync. Reshell (opt-in) features are documented on
  `client/workbench.md` (Экспериментальный интерфейс).
- **macOS runs are for validation only** (offscreen platform + hermetic
  `HOME`; see "Running on macOS" in `docs/ops/client-screenshots.md`); published
  images come from the Windows pipeline so fonts stay consistent.

### UX design system

The client's UX design system lives under [`docs/client/ux/`](../docs/client/ux/README.md).
**Read it before adding or restyling any UI**, and follow it rather than
inventing chrome.

**The direction is a native desktop look and feel** (agreed 2026-07-26,
superseding the earlier browser-styled reshell). The client must read as a
native application on each host OS:

- **Use the platform Qt style** — `windows11`/`windowsvista`, `macos`, or the
  Linux `QT_QPA_PLATFORMTHEME` style. Do **not** call
  `QApplication::setStyle("Fusion")`; the platform default wins and the `Style`
  QSetting is a user override.
- **Colour through `QPalette` roles**, not stylesheets. Adding a
  `setStyleSheet` with a baked colour is a regression — there are ~105 such
  sites and the plan is to remove them (backlog P6), not add more.
- **Size through `QStyle::PixelMetric`, `QFontMetrics`, `QApplication::font()`.**
  Never hard-code `font-size:Npx`, fixed widget widths, or hand-tuned radii:
  they break DPI scaling and the OS font-size accessibility setting.
- **Follow the OS light/dark preference by default**, with an explicit
  dark/light/high-contrast override. Theme changes must apply live, which means
  handling `QEvent::ApplicationPaletteChange`.
- **Prefer stock Qt widgets in their conventional roles** — `QMenuBar`,
  `QToolBar`, `QDockWidget`, `QStatusBar`, `QMessageBox`, `QFileDialog`. Native
  dialogs are the desired end state, not something to theme away.
- **Dialogs follow [`docs/client/ux/dialogs.md`](../docs/client/ux/dialogs.md)**. Two rules catch
  most defects: the OS title bar *is* the title, so never repeat it (or a brand
  mark) in the content area; and always use `QDialogButtonBox` rather than
  laying out OK/Cancel by hand — button order is opposite on macOS and Windows,
  and the box also supplies Qt's own translated labels.
- **Exception — process semantics are ours, not the platform's.** Alarm
  severity, data quality (good/uncertain/bad), and single-line equipment state
  are functional safety colours (ISA-101, ISA-18.2/EEMUA 191). They keep their
  fixed token values, must not follow the OS accent, and must not invert with
  the system theme. Colour is still never the only signal — always pair with a
  label or shape.

The information architecture from the reshell still stands (Activity bar →
Explorer → workspace tabs → Inspector → status strip, operator-first, one home
per datum). It is the *chrome* that becomes native, not the layout.

The HTML mockups in [`docs/product/ui-mockups/screens/`](../docs/product/ui-mockups/screens/) are
**the north star for information architecture** — which surfaces exist, what
each one shows, and what the operator can act on there. They were restyled to
this native direction on 2026-07-26, but they remain approximations of it and
are **not a visual target**: "native" means the appearance is the host
platform's, and it differs between macOS and Windows. Read them for *what goes
where and which data appears*; take the *appearance* from the platform, and
validate implemented UI against real Qt widgets via the headless
`client_screenshot_generator`, never against the HTML. See
[`docs/client/ux/README.md`](../docs/client/ux/README.md) → Mockups for the full caveat.

That architecture is the **product's**, not just this client's — the web client
implements the same one, rendered in its own idiom. The cross-client obligation
that goes with it is recorded in the superproject `CLAUDE.md`, not here (this
repo must stay standalone).

- [`docs/client/ux/principles.md`](../docs/client/ux/principles.md) — HMI/SCADA UX principles
  (High-Performance HMI, ISA-101, ISA-18.2/EEMUA 191 alarms, situational
  awareness, colour rules) with citations. The *why* behind every UI decision.
- [`docs/client/ux/design-language.md`](../docs/client/ux/design-language.md) — the shared
  design tokens (exact colour/type/spacing values) and component primitives.
  Components must consume tokens; never hard-code hex.
- [`docs/client/ux/shell.md`](../docs/client/ux/shell.md) — the reshelled layout mapped onto the
  existing `main_window/` / registries / `modules/` code.
- [`docs/client/ux/backlog.md`](../docs/client/ux/backlog.md) — the surface catalogue and
  dependency notes. Treat it as a **menu of slices, not a fixed waterfall**
  (see the implementation approach below).
- Rendered, theme-toggleable mockups live in
  [`docs/product/ui-mockups/screens/`](../docs/product/ui-mockups/screens/) — operator,
  engineering and admin surfaces, each with a light/dark toggle. They are the
  architecture reference described above, not a visual target.

### UX implementation approach

**Do not follow `backlog.md` as a strict P0→P5 waterfall.** The agreed way to
build the reshell (decided with the user) is:

- **Incremental vertical slices.** Ship one coherent surface end-to-end at a
  time, each independently valuable, and re-evaluate after each. Do not attempt
  a big-bang switchover — this is a live client edited by multiple people.
- **Full reshell is the north star, reached gradually.** The operator-workbench
  structure (Activity bar → Explorer → workspace tabs → Inspector → status
  strip) remains the target; slices converge on it rather than landing it all
  at once.
- **Theming is opt-in and palette-first.** The design-token theming
  (`scada::aui::ApplyTheme` in [`aui/qt/theme_qt.h`](aui/qt/theme_qt.h)) is
  **off by default**. The operator picks it in **Settings → Colour scheme**
  (`AppearanceMenuModel` in `main_window/main_menu/`), a radio menu alongside
  Settings → Style that applies live; `ClearTheme()` is its inverse and takes
  the client back to the untouched platform look. The choice is restored and
  persisted by `InstalledAppearance` (`app/qt/installed_appearance.h`) from the
  `Ux/Experimental` + `Ux/Theme` QSettings — the menu is the only way in, so
  don't add a second one (a `SCADA_UX_EXPERIMENTAL` env override existed only
  while there was no UI, and was dropped with it).
  `ApplyTheme`/`ClearTheme` own the severity ramp too — never set
  `SetSeverityTheme` alongside them, which is how three copies of that mapping
  drifted. Prefer recolouring through `QPalette`
  (`ThemeScope::kPaletteOnly`). The global stylesheet (`kFull`) has been
  **reduced to a single rule** (backlog P6.2) — everything a native style can
  draw is now left to it. `BuildThemeStyleSheet` records what was removed and
  why; a unit test fails if any of those selectors comes back. Never grow the
  global sheet, and never make it unconditional.
- **Every removed `setStyleSheet` is progress.** Adding one needs a reason that
  `QPalette` plus `QStyle::PixelMetric` could not serve — state it in a comment.
- **Validate by purpose, and validate in both OS appearances.** Use the HTML
  mockups in `docs/product/ui-mockups/screens/` for information architecture only;
  validate anything implemented against **real Qt widgets** via the headless
  `client_screenshot_generator` (see `docs/ops/client-screenshots.md`) — not HTML, which
  does not match Qt's rendering. A native-look change is not done until it has
  been seen under both a light and a dark system theme.

When in doubt about scope or sequence, ask rather than executing the backlog
top to bottom.

### When to update the docs

**Update `docs/client/design.md`, `docs/client/requirements.md`, and the relevant diagram
whenever you change or add functionality.** Concretely, that means at
minimum:

- Adding or removing a top-level module (`*_module.{h,cpp}`) — update the
  module table and `docs/diagrams/client-module-graph.puml`.
- Adding or removing a directory under `client/` that hosts a new layer or
  domain area — update the layer description and
  `docs/diagrams/client-architecture-layers.puml`.
- Adding or removing a back-end registered with `REGISTER_DATA_SERVICES` —
  update FR-1 in `docs/client/requirements.md` §3.
- Changing the bootstrap order in `ClientApplication::PostLogin()` —
  update `docs/diagrams/client-bootstrap-sequence.puml`.
- Adding a new actor-facing capability that isn't covered by an existing
  use case — add a row to the use-case table in `docs/client/requirements.md` §2
  and a functional requirement in `docs/client/requirements.md` §3.
- Removing a use case (deleting a feature) — strike the row in §2 and the
  matching FR.
- Changing a design token, component primitive, or a shell region — update the
  matching `docs/client/ux/` doc **and** the affected mockup in
  `docs/product/ui-mockups/screens/` in the same change, then regenerate the touched
  `screenshots/` image once the code lands.

If you cannot tell whether a change affects the doc, ask. Drift between
the doc and the code is worse than no doc.

### Diagrams

Architecture diagrams live in the superproject's shared `docs/diagrams/`
tree, prefixed `client-` to distinguish them from the server ones. Paths in
this section are relative to the superproject root.

| File | Renders to | Used in |
|---|---|---|
| `docs/diagrams/client-use-cases.puml` | `client-use-cases.svg` | `docs/client/requirements.md` §2 (use cases) |
| `docs/diagrams/client-architecture-layers.puml` | `client-architecture-layers.svg` | `docs/client/design.md` §3 (component overview) |
| `docs/diagrams/client-module-graph.puml` | `client-module-graph.svg` | `docs/client/design.md` §3.6 (domain modules) |
| `docs/diagrams/client-bootstrap-sequence.puml` | `client-bootstrap-sequence.svg` | `docs/client/design.md` §3.1 (startup sequence) |
| `docs/diagrams/client-opcua-discovery-flow.puml` | `client-opcua-discovery-flow.svg` | `docs/client/opcua-client-interop.md` |

The `.svg` files are committed alongside the `.puml` sources so the doc
renders correctly on GitHub without a build step.

**To update a diagram:**

1. Edit the corresponding `.puml` file. PlantUML syntax reference:
   <https://plantuml.com/>.
2. Regenerate the SVG with `plantuml` (macOS: `brew install plantuml`; it
   brings its own JDK and Graphviz):

   ```bash
   plantuml -tsvg docs/diagrams/client-<name>.puml
   ```

3. **Look at the rendered output** before committing — render a PNG
   (`plantuml -tpng docs/diagrams/client-<name>.puml -o /tmp`) and open it. Layout
   collisions and PlantUML warning banners are drawn *into* the image and
   are invisible in the source.
4. Commit both the `.puml` source *and* the regenerated `.svg`. They must
   stay in lock-step — never commit one without the other.

**PlantUML conventions worth knowing:**

- Start every diagram with `!include _style.puml` — the shared house style
  (theme, skinparams, palette variables `$tier`/`$config`/`$store`/`$hazard`/
  `$external`/`$neutral`/`$proxy`) lives in `docs/diagrams/_style.puml`. See the
  superproject `CLAUDE.md`, "PlantUML house style", for the palette table.
- Colour an activity with `:text; <<$tier>>` **after** the semicolon. The
  legacy `#RRGGBB:text;` prefix form is deprecated and PlantUML draws a
  warning banner into the image.
- `<style>` blocks with stereotype classes silently do nothing under
  `!theme plain` — colours just don't apply, with no error.
- A trailing `' comment` on a `!$var = "..."` line is a syntax error; put
  preprocessor comments on their own lines.
- Always end the file with a trailing newline.

**To add a new diagram:**

1. Create `docs/diagrams/client-<name>.puml` starting with
   `!include _style.puml`.
2. Render it as above, and look at the PNG.
3. Reference it from `docs/client/design.md` with
   `![alt](../diagrams/client-<name>.svg)` and a "Source: …" caption pointing
   back to the `.puml`.
4. Add it to the table above in this section.

## Build System

### CMake (Primary — Cross-Platform)

The project uses CMake with a hierarchical structure. Each module directory has its own `CMakeLists.txt`.

**CMake Presets:**

The shared `CMakePresets.json` defines a single `ninja` configure preset. Developers create a `CMakeUserPresets.json` (git-ignored) with local paths, MSVC environment, and dev presets that inherit from `ninja`. See `CMakeUserPresets.json.template` for the template.

| Type | Preset | Description |
| ---- | ------ | ----------- |
| Configure | `ninja-dev` | Inherits `ninja`, adds MSVC environment and local paths |
| Build | `debug-dev` | Debug build |
| Build | `release-dev` | RelWithDebInfo build |
| Test | `test-release-dev` | Runs tests (RelWithDebInfo) |
| Test | `test-debug-dev` | Runs tests (Debug) |

```bash
cmake --preset ninja-dev                    # Configure (once)
cmake --build --preset release-dev          # Build (RelWithDebInfo)
cmake --build --preset debug-dev            # Build (Debug)
ctest --preset test-release-dev             # Test (RelWithDebInfo)
ctest --preset test-debug-dev               # Test (Debug)
```

### MSBuild (Windows Only)

```bash
nuget restore .
msbuild /m /p:Configuration=Release .
```

### Custom CMake Module System

The `aui/client_module.cmake` file (owned by aui, which is slated for extraction into its own repository; the client gets it via `find_package(ScadaClientAui)`) defines helper functions for the dual Qt/Wt build architecture. Every module creates two targets (`<name>_qt` and `<name>_wt`) automatically:

- `client_module(name)` — Creates both Qt and Wt library targets
- `client_module_sources(name PUBLIC|PRIVATE dirs...)` — Adds sources from directories (auto-includes `dir/qt/` and `dir/wt/` subdirs)
- `client_module_link_libraries(name PUBLIC|PRIVATE libs...)` — Links libraries, auto-resolving `_qt`/`_wt` suffixed targets
- `client_module_include_directories(name PUBLIC|PRIVATE dirs...)` — Adds include directories

Qt targets get `AUTOMOC`, `AUTOUIC`, `AUTORCC` enabled and `.ts` translation files processed automatically.

### Key Dependencies

Managed via `vcpkg.json` manifest:

- **Qt 6** — `qtbase` (Widgets, PrintSupport), `qttools` (LinguistTools), `qtactiveqt` (Windows)
- **Boost** — `boost-asio`, `boost-beast`, `boost-signals2`, `boost-locale`, `boost-range`, `boost-algorithm`
- **Google Test** — `gtest`
- **Wt** — `wt` (web framework for alternative UI)

Not managed by vcpkg:

- **OPC UA SDK** — Industrial protocol (via `third_party/opc`)
- **Modus 6.30** — ActiveX/COM library (Windows/Qt only, via `${deps}/modus`)
- **Windows SDK / ATL** — COM/ActiveX support (Windows only)

## CMake Options

| Option | Default | Description |
| ------ | ------- | ----------- |
| `BUILD_OPC` | `ON` | Build Classic OPC modules in scada-common (Windows only) |
| `BUILD_VIDICON` | `ON` | Build Vidicon modules in scada-common (Windows only) |

The `vidicon` client module is automatically skipped when `scada_common_opc` and `scada_common_vidicon` targets are not available.

## C++20 Module Facades (SCADA_CXX_MODULES)

With `-DSCADA_CXX_MODULES=ON` (default OFF, build unchanged when OFF), the
client library layers expose named-module facades (`scada.client.base`,
`scada.client.aui`, `scada.client.controller`, ...) following the core/common
facade design. The client set is Qt-flavored — only the `_qt` targets are
facaded; the wt flavor stays header-based. See `docs/ops/client-cxx-modules.md` for the
module map, exclusions, and presets, and `core/docs/cxx-modules.md` for the
underlying design and consumer rules.

## CI/CD

GitHub Actions workflow (`.github/workflows/cmake-multi-platform.yml`) triggered on pushes/PRs to `release/2.5`.

**Matrix:** Windows x64, Windows x86, Ubuntu GCC, Ubuntu Clang.

**How it works:** CI checks out dependency repos (`scada-core`, `scada-common`, `transport`, `chromebase`, `express`, `graph-qt`, `opcuapp`, `UA-AnsiC`) as sibling directories and uses `cmake --preset ninja` with `-D` overrides for `CMAKE_MODULE_PATH` and other settings. Modules requiring proprietary SDKs (`BUILD_OPC=OFF`, `BUILD_VIDICON=OFF`) are disabled. The legacy promise dependency is resolved by `scada-core`, not by the client preset.

```bash
# CI build commands (for reference):
cmake --preset ninja -DCMAKE_MODULE_PATH="..." -DBUILD_OPC=OFF ...
cmake --build build/ninja --config RelWithDebInfo
ctest --test-dir build/ninja --build-config RelWithDebInfo --output-on-failure
```

## Architecture

### Module-Based MVC with Dependency Injection

The application follows a modular MVC architecture with explicit context-based dependency injection.

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

### Platform Abstraction (Qt/Wt)

Each module that has UI splits code into:
- **Shared model code** — in the module root directory
- **`qt/` subdirectory** — Qt-specific implementation
- **`wt/` subdirectory** — Wt-specific implementation

The `UI_WT` preprocessor macro distinguishes builds. Modus and Vidicon modules are Qt-only (`#if !defined(UI_WT)`).

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

### Modern C++ (C++17+)

- Smart pointers throughout (`std::unique_ptr`, `std::shared_ptr`) — no raw `new`/`delete`
- Designated initializers for context structs: `.field_ = value`
- `std::optional<T>` for optional return values
- `std::string_view` for non-owning string parameters
- `std::ranges` and `std::views` for range pipelines
- Structured bindings
- `[[nodiscard]]` on functions returning promises/important values
- `= delete` for non-copyable classes
- `std::function` for callbacks
- `std::bind_front` for partial application
- Move semantics for context passing: `explicit Module(Context&& context)`
- `using namespace std::chrono_literals` for duration literals (`1min`)

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
- Public and module-crossing async APIs may keep returning `promise<T>` for
  compatibility, but client implementation code should be coroutine-first.
- Use `AwaitPromise(...)` and `ToPromise(...)` only at legacy boundaries
  instead of adding `.then()` chains.

### Localization

- UI strings use `u"..."` (UTF-16 string literals) for Russian text
- Qt `.ts` translation files in `qt/` subdirectories
- Translation files: `*_ru.ts`
- **Never run `lupdate` against `app/qt/client_ru.ts`.** Most client UI strings
  go through the custom `Translate()` helper, which `lupdate` does not
  recognise as a translation call — a refresh would mark every one of them
  `vanished`, and `lrelease` drops those, so they would silently stop shipping.
  Maintain that file by hand. To get an authoritative string list for a form,
  run `lupdate` over just that form into a scratch `.ts` and merge the result
  in.
- **Never hard-code a user-facing string as a `u"..."` literal** — it can never
  be translated, and no other check sees it (`lupdate` does not recognise
  `Translate()`, and a missing lookup falls back silently to English).
  `client_untranslated_string_check` (ctest, `tools/check_untranslated_ui_strings.py`)
  fails on a literal reaching a message box, a `ResourceError`, a file-dialog
  title or `setWindowTitle`. Note a namespace-scope
  `const char16_t k…[] = u"…"` *cannot* call `Translate()` at all — it needs a
  running QApplication — so make such titles functions returning
  `std::u16string`.
- **Never write Russian in a string literal.** The same check's second rule
  fails on Cyrillic in *any* client string literal, decoding `\uXXXX` escapes
  first (escaping is how these hid from a grep). It exists because the sink
  rule above only sees six call shapes and is blind to the larger
  population — a status-strip cell, a menu caption, a grid placeholder never
  reach a message box, so a Russian literal sat in each of them permanently
  untranslatable. Russian in a **comment** is fine and is not reported;
  `modules/modus/activex/` is allowlisted because those OLESTR names are the
  Vidicon ActiveX protocol's own identifiers.
  Two habits keep the sweep honest: pick an English source string that is not
  already in the empty context of `client_ru.ts` with a *different* Russian
  translation (`Translate()` has no context to disambiguate with — this is why
  the tab context menu says `To Favourites` rather than reusing
  `Add to Favourites`), and preserve the Russian exactly as it displayed, so
  the change is invisible to the operator and to the screenshot gallery.
- **The same rule now covers `core/` and `common/`, which is where the rest of
  the operator's text comes from.** Status-code descriptions, data-quality
  flags and the boolean Yes/No labels are produced below the client and
  rendered by it verbatim; they carry English sources and go through
  `scada::TranslateUiText` (`core/base/ui_text.h`), whose translator the client
  installs in `AppInit`. Their Russian lives in `app/qt/client_ru.ts` like
  everything else, so adding a status code means adding a catalog entry.
  Without a translator installed — every server tier, and every unit test — the
  English source renders, and that is the intended behaviour, not a fallback.
  These trees are optional to the check: this repo publishes standalone, so it
  skips whichever of them is absent.
- A `.ui` form's strings belong to the **form class's context**
  (`uic` emits `QCoreApplication::translate("<FormClass>", ...)`), not the
  empty context. `client_ui_translation_check` (ctest, see
  `tools/check_ui_translations.py`) fails the build when a form string has no
  translation that would reach the `.qm`; it also records the deliberate
  exclusions and the remaining untranslated strings.

## Testing

### Test Framework

Unit tests follow the `*_unittest.cpp` naming convention (33 test files). Tests are located alongside the source code they test. Test execution uses CTest.

### Test Locations (examples)

- `app/client_application_unittest.cpp`
- `modules/table/table_model_unittest.cpp`
- `modules/events/event_table_model_unittest.cpp`
- `main_window/main_window_unittest.cpp`
- `profile/page_layout_unittest.cpp`

### Reusable Test Helpers

Put reusable test helpers — shared fakes, fixture bases, mock-factory
functions, and test-only utilities used by more than one module — under
`client/test/`. Keep per-module `*_unittest.cpp` files focused on the
tests themselves; when a helper starts getting copy-pasted across
modules, move it into `client/test/` and link the module's test target
against it.

### Running Tests

```bash
ctest --preset test-release-dev             # RelWithDebInfo
ctest --preset test-debug-dev               # Debug
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

1. **New modules** should follow the Context + private inheritance pattern. Define a `*Context` struct, have the module privately inherit from it, and accept `Context&&` in the constructor.

2. **New UI components** need both `qt/` and `wt/` subdirectories. Shared logic goes in the module root; platform-specific code goes in the respective subdirectory. Use `client_module()` in CMake.

3. **New controllers** should be registered via `REGISTER_CONTROLLER(ControllerClass, window_info)` or dynamically through `ControllerRegistry`.

4. **Tests** should be placed alongside source as `*_unittest.cpp` files.

5. **Member variables** always end with a trailing underscore.

6. **Headers** always use `#pragma once`.

7. **Dependencies** between modules are explicit through Context struct fields — avoid hidden globals.

8. **Destruction order matters** — `ClientApplication::~ClientApplication()` resets members in a specific order to respect dependency chains. Follow this pattern when adding new modules.

9. **Conditional compilation** — Use `#if !defined(UI_WT)` to guard Qt-only features (Modus, Vidicon, etc.).

13. **`aui/` is slated for extraction into its own repository.** Never add
    includes of client-repo headers (`profile/`, `resources/`, `ui/`,
    `modules/`, `main_window/`, …) or scada-common dependencies inside
    `aui/` — invert the dependency instead (keep the generic seam in aui,
    move the client-coupled piece to its consumer). Its allowed dependency
    set is `scada_base`, `graph_qt`, `view_manager_qt`, Qt/Wt — see
    `docs/client/aui-extraction.md`.

11. **Modus/Vidicon ActiveX parameter names** — Never rename OLESTR parameter names in `modules/modus/` (e.g., `"ключ_привязки"`, `"положение"`, `"уставки"`). These Russian-language identifiers are part of the external Vidicon ActiveX protocol interface and must remain unchanged.

10. **Async code** keeps `promise<T>` at public/module boundaries, but new or touched implementation code should use coroutine bodies with `co_await`. Use `AwaitPromise(...)` to await legacy promises and `ToPromise(...)` only at compatibility boundaries; do not add new `.then()` chains for client workflows.

12. **Add a regression unit test for every fixed bug.** When fixing a bug, add a `*_unittest.cpp` test that fails against the pre-fix code and passes after the fix — it locks in the fix and documents the failure mode in an executable form. If writing the test would require a major redesign (e.g., a new mock layer, restructuring the class under test, splitting a module), confirm the scope with the user before embarking; a targeted regression test on existing seams is always preferable to invasive test plumbing.
