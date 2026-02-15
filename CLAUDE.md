# CLAUDE.md — Telecontrol SCADA Client

## Project Overview

Telecontrol SCADA Client is a C++ industrial monitoring and control application (version 2.5.6). It provides remote device monitoring, real-time and historical data viewing, event/alarm journaling, and device configuration management. The application supports multiple industrial protocols (SCADA/Telecontrol, OPC UA, Vidicon, Modus) and ships with dual UI frontends: Qt 5 (desktop) and Wt (web).

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
├── components/             # ~22 reusable UI components
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
│   └── ...                 # (select_item, node_properties, node_table, etc.)
├── configuration/          # Device/node configuration management
│   ├── devices/
│   ├── nodes/
│   ├── objects/
│   └── tree/
├── controller/             # MVC controller layer, view management, command registry
│   └── test/
├── core/                   # Core module: command registries, tracer, progress host
├── events/                 # Event system, event journal, local events
├── export/                 # Export functionality
│   ├── configuration/      # Configuration export/import
│   └── csv/                # CSV export
├── favorites/              # Bookmarks/favorites management
├── filesystem/             # File system operations and caching
├── graph/                  # Graph/chart visualization
├── main_window/            # Main window management and page/view lifecycle
├── modus/                  # Modus 6.30 ActiveX/COM protocol integration (Qt only)
├── portfolio/              # Portfolio management
├── print/                  # Print and print preview
├── profile/                # User profiles, window definitions, page layouts
├── properties/             # Property management and dialogs
├── services/               # Shared services (speech, task management, telemetry)
├── timed_data/             # Time-series data service
├── vidicon/                # Vidicon protocol integration (Qt only)
├── web/                    # Web component
├── res/                    # Resources and settings
├── test/                   # Integration tests and display tester
├── .github/workflows/      # CI: cmake-multi-platform.yml, msbuild.yml
├── CMakeLists.txt          # Root CMake build file
├── client_module.cmake     # Custom CMake helpers for dual Qt/Wt target creation
├── translation.cmake       # Qt translation support
├── common.rc               # Windows resource definitions
├── common_resources.h      # Resource IDs and constants
├── client_utils.cpp/.h     # Global utility functions
├── tasks.md                # Bug/feature backlog
└── command-line.md         # Command-line switch documentation
```

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

The `client_module.cmake` file defines helper functions for the dual Qt/Wt build architecture. Every module creates two targets (`<name>_qt` and `<name>_wt`) automatically:

- `client_module(name)` — Creates both Qt and Wt library targets
- `client_module_sources(name PUBLIC|PRIVATE dirs...)` — Adds sources from directories (auto-includes `dir/qt/` and `dir/wt/` subdirs)
- `client_module_link_libraries(name PUBLIC|PRIVATE libs...)` — Links libraries, auto-resolving `_qt`/`_wt` suffixed targets
- `client_module_include_directories(name PUBLIC|PRIVATE dirs...)` — Adds include directories

Qt targets get `AUTOMOC`, `AUTOUIC`, `AUTORCC` enabled and `.ts` translation files processed automatically.

### Key Dependencies

Managed via `vcpkg.json` manifest:

- **Qt 5** — `qt5-base` (Widgets, PrintSupport), `qt5-tools` (LinguistTools), `qt5-activeqt` (Windows), `qt5-winextras` (Windows)
- **Boost** — `boost-asio`, `boost-beast`, `boost-signals2`, `boost-locale`, `boost-range`, `boost-algorithm`
- **Google Test** — `gtest`
- **Wt** — `wt` (web framework for alternative UI)

Not managed by vcpkg:

- **OPC UA SDK** — Industrial protocol (via `third_party/opc`)
- **Modus 6.30** — ActiveX/COM library (Windows/Qt only, via `${deps}/modus`)
- **Windows SDK / ATL** — COM/ActiveX support (Windows only)

## CI/CD

GitHub Actions workflow triggered on pushes/PRs to `release/2.5`:

- **cmake-multi-platform.yml** — Uses CMake presets (Ninja Multi-Config) with vcpkg across Windows (MSVC), Ubuntu (GCC), Ubuntu (Clang); runs configure, build, and test steps

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
  std::shared_ptr<Executor> executor_;
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
| `EventModule` | `events/event_module.h` | Event fetching, journaling, local events |
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

- Async operations return `promise<T>` (custom promise type)
- Exceptions for fatal errors (`std::runtime_error`)
- Local event system (`LocalEvents`) for user-visible errors/warnings

### Concurrency

- `boost::asio::io_context` for async I/O
- `Executor` abstraction for task scheduling
- `BindPromiseExecutor` to pin continuations to the correct executor
- `promise<T>` for async result chaining

### Localization

- UI strings use `u"..."` (UTF-16 string literals) for Russian text
- Qt `.ts` translation files in `qt/` subdirectories
- Translation files: `*_ru.ts`

## Testing

### Test Framework

Unit tests follow the `*_unittest.cpp` naming convention (33 test files). Tests are located alongside the source code they test. Test execution uses CTest.

### Test Locations (examples)

- `app/client_application_unittest.cpp`
- `components/table/table_model_unittest.cpp`
- `events/event_table_model_unittest.cpp`
- `main_window/main_window_unittest.cpp`
- `profile/page_layout_unittest.cpp`

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

10. **Async code** uses `promise<T>` with `.then()` chaining and `BindPromiseExecutor` to stay on the correct executor thread.
