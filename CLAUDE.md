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
├── docs/                   # Design doc and architecture diagrams
│   ├── design.md           # High-level architecture and component design
│   ├── requirements.md     # Use cases and FR/NFR traceability
│   ├── *.mmd               # Mermaid diagram sources
│   └── *.svg               # Generated diagrams referenced from design.md
├── .github/workflows/      # CI: cmake-multi-platform.yml, msbuild.yml
├── CMakeLists.txt          # Root CMake build file
├── aui/client_module.cmake # Custom CMake helpers for dual Qt/Wt target creation (aui-owned)
├── translation.cmake       # Qt translation support
├── common.rc               # Windows resource definitions
├── common_resources.h      # Resource IDs and constants
├── client_utils.cpp/.h     # Global utility functions
├── tasks.md                # Bug/feature backlog
└── docs/command-line.md    # Command-line switch documentation
```

## Documentation

The high-level design document lives at `docs/design.md`. It captures the
layered component architecture, all grounded in concrete source files. Use
`docs/requirements.md` for use cases, functional requirements, and
non-functional requirements. Treat both as living documents, not snapshots.

### UX design system

The client's UX design system lives under [`docs/ux/`](docs/ux/README.md).
**Read it before adding or restyling any UI**, and follow it rather than
inventing chrome. The direction is a full reshell to an operator workbench
(Activity bar → Explorer → workspace tabs → Inspector → status strip),
visually consistent with the web client, using shared light/dark/high-contrast
design tokens (desktop defaults to dark).

- [`docs/ux/principles.md`](docs/ux/principles.md) — HMI/SCADA UX principles
  (High-Performance HMI, ISA-101, ISA-18.2/EEMUA 191 alarms, situational
  awareness, colour rules) with citations. The *why* behind every UI decision.
- [`docs/ux/design-language.md`](docs/ux/design-language.md) — the shared
  design tokens (exact colour/type/spacing values) and component primitives.
  Components must consume tokens; never hard-code hex.
- [`docs/ux/shell.md`](docs/ux/shell.md) — the reshelled layout mapped onto the
  existing `main_window/` / registries / `modules/` code.
- [`docs/ux/backlog.md`](docs/ux/backlog.md) — the surface catalogue and
  dependency notes. Treat it as a **menu of slices, not a fixed waterfall**
  (see the implementation approach below).
- Rendered, theme-toggleable mockups (the visual source of truth) live in
  [`docs/ui-mockups/screens/`](docs/ui-mockups/screens/) — operator and
  engineering surfaces, each with a light/dark toggle.

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
  **off by default** — it only runs when the `Ux/Experimental` QSetting is true
  (`app/qt/main.cpp`), so the legacy Fusion look is unchanged for everyone else.
  Prefer recolouring through `QPalette` (`ThemeScope::kPaletteOnly`) and
  targeted per-widget styling; the global stylesheet (`kFull`) is additive and
  must be validated against ActiveX (Modus/Vidicon) and custom-painted widgets
  (the graph) before it is relied on. Do not make theming unconditional or grow
  one monolithic global sheet.
- **Validate by purpose.** Use HTML mockups in `docs/ui-mockups/` for
  brand-new layouts; validate anything actually implemented against **real Qt
  widgets** via the headless `client_screenshot_generator` (see
  `docs/screenshots.md`) — not HTML, which does not match Qt's rendering.

When in doubt about scope or sequence, ask rather than executing the backlog
top to bottom.

### When to update the docs

**Update `docs/design.md`, `docs/requirements.md`, and the relevant diagram
whenever you change or add functionality.** Concretely, that means at
minimum:

- Adding or removing a top-level module (`*_module.{h,cpp}`) — update the
  module table and `module-graph.mmd`.
- Adding or removing a directory under `client/` that hosts a new layer or
  domain area — update the layer description and `architecture-layers.mmd`.
- Adding or removing a back-end registered with `REGISTER_DATA_SERVICES` —
  update FR-1 in `docs/requirements.md` §3.
- Changing the bootstrap order in `ClientApplication::PostLogin()` —
  update `bootstrap-sequence.mmd`.
- Adding a new actor-facing capability that isn't covered by an existing
  use case — add a row to the use-case table in `docs/requirements.md` §2
  and a functional requirement in `docs/requirements.md` §3.
- Removing a use case (deleting a feature) — strike the row in §2 and the
  matching FR.
- Changing a design token, component primitive, or a shell region — update the
  matching `docs/ux/` doc **and** the affected mockup in
  `docs/ui-mockups/screens/` in the same change, then regenerate the touched
  `docs/screenshots/` image once the code lands.

If you cannot tell whether a change affects the doc, ask. Drift between
the doc and the code is worse than no doc.

### Diagrams

Architecture diagrams live next to `design.md` as Mermaid sources:

| File | Renders to | Used in design.md §|
|---|---|---|
| `docs/use-cases.mmd` | `use-cases.svg` | `requirements.md` §2 (use cases) |
| `docs/architecture-layers.mmd` | `architecture-layers.svg` | `design.md` §3 (component overview) |
| `docs/module-graph.mmd` | `module-graph.svg` | `design.md` §3.6 (domain modules) |
| `docs/bootstrap-sequence.mmd` | `bootstrap-sequence.svg` | `design.md` §3.1 (startup sequence) |

The `.svg` files are committed alongside the `.mmd` sources so the doc
renders correctly on GitHub without a build step.

**To update a diagram:**

1. Edit the corresponding `.mmd` file. Mermaid syntax reference:
   <https://mermaid.js.org/intro/>.
2. Regenerate the SVG with `mmdc` (mermaid-cli, installed globally via
   `npm install -g @mermaid-js/mermaid-cli`):

   ```bash
   cd client/docs
   mmdc -i <name>.mmd -o <name>.svg -b transparent
   ```

3. Commit both the `.mmd` source *and* the regenerated `.svg`. They must
   stay in lock-step — never commit one without the other.

**Mermaid quirks worth knowing:**

- Sequence-diagram message text cannot contain `,` `;` `&` HTML entities,
  or PascalCase identifiers at the very end of a line — the parser
  interprets them as new statements. Rephrase or split.
- Flowchart node labels accept HTML (`<b>`, `<i>`, `<br/>`); sequence
  participant labels do not.
- Always end the file with a trailing newline.

**To add a new diagram:**

1. Create `docs/<name>.mmd`.
2. Render it as above.
3. Reference it from `design.md` with `![alt](<name>.svg)` and a "Source:
   …" caption pointing back to the `.mmd`.
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
| `BUILD_MODUS` | `ON` | Build Modus module (requires Modus SDK) |
| `BUILD_OPC` | `ON` | Build Classic OPC modules in scada-common (Windows only) |
| `BUILD_VIDICON` | `ON` | Build Vidicon modules in scada-common (Windows only) |

The `vidicon` client module is automatically skipped when `scada_common_opc` and `scada_common_vidicon` targets are not available.

## C++20 Module Facades (SCADA_CXX_MODULES)

With `-DSCADA_CXX_MODULES=ON` (default OFF, build unchanged when OFF), the
client library layers expose named-module facades (`scada.client.base`,
`scada.client.aui`, `scada.client.controller`, ...) following the core/common
facade design. The client set is Qt-flavored — only the `_qt` targets are
facaded; the wt flavor stays header-based. See `docs/cxx-modules.md` for the
module map, exclusions, and presets, and `core/docs/cxx-modules.md` for the
underlying design and consumer rules.

## CI/CD

GitHub Actions workflow (`.github/workflows/cmake-multi-platform.yml`) triggered on pushes/PRs to `release/2.5`.

**Matrix:** Windows x64, Windows x86, Ubuntu GCC, Ubuntu Clang.

**How it works:** CI checks out dependency repos (`scada-core`, `scada-common`, `transport`, `chromebase`, `express`, `graph-qt`, `opcuapp`, `UA-AnsiC`) as sibling directories and uses `cmake --preset ninja` with `-D` overrides for `CMAKE_MODULE_PATH` and other settings. Modules requiring proprietary SDKs (`BUILD_MODUS=OFF`, `BUILD_OPC=OFF`, `BUILD_VIDICON=OFF`) are disabled. The legacy promise dependency is resolved by `scada-core`, not by the client preset.

```bash
# CI build commands (for reference):
cmake --preset ninja -DCMAKE_MODULE_PATH="..." -DBUILD_MODUS=OFF ...
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
    `docs/aui-extraction.md`.

11. **Modus/Vidicon ActiveX parameter names** — Never rename OLESTR parameter names in `modules/modus/` (e.g., `"ключ_привязки"`, `"положение"`, `"уставки"`). These Russian-language identifiers are part of the external Vidicon ActiveX protocol interface and must remain unchanged.

10. **Async code** keeps `promise<T>` at public/module boundaries, but new or touched implementation code should use coroutine bodies with `co_await`. Use `AwaitPromise(...)` to await legacy promises and `ToPromise(...)` only at compatibility boundaries; do not add new `.then()` chains for client workflows.

12. **Add a regression unit test for every fixed bug.** When fixing a bug, add a `*_unittest.cpp` test that fails against the pre-fix code and passes after the fix — it locks in the fix and documents the failure mode in an executable form. If writing the test would require a major redesign (e.g., a new mock layer, restructuring the class under test, splitting a module), confirm the scope with the user before embarking; a targeted regression test on existing seams is always preferable to invasive test plumbing.
