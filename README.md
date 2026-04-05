# Telecontrol SCADA Client

A C++ industrial SCADA (Supervisory Control and Data Acquisition) client application for remote monitoring and control of industrial systems. Provides real-time and historical data viewing, event/alarm journaling, device configuration management, and support for multiple industrial protocols.

## Features

- Remote device monitoring and control
- Real-time and historical time-series data viewing
- Event/alarm journaling with acknowledgment
- Device and node configuration management
- User authentication and persistent profiles
- Multi-window interface with customizable page layouts
- Data export (CSV, configuration)
- Print and print preview
- Graph/chart visualization
- Protocol support: SCADA/Telecontrol, OPC UA, Vidicon, Modus
- Dual UI: Qt 5 desktop application and Wt web application

## Prerequisites

- C++17 compiler (MSVC, GCC, or Clang)
- CMake 3.x+
- [vcpkg](https://vcpkg.io/) (recommended for dependency management)
- Qt 5 (Widgets, LinguistTools, PrintSupport; ActiveQt and WinExtras on Windows)
- Boost (ASIO, Beast, Signals2, Locale, Range, Algorithm)
- Google Test
- Wt (web UI framework)
- OPC UA SDK (via `third_party/opc`)
- Windows SDK / ATL (Windows only, for Modus and COM support)

## Building

### CMake Presets (Recommended)

The shared `CMakePresets.json` defines a `ninja` configure preset (Ninja Multi-Config). Developers create a `CMakeUserPresets.json` (git-ignored) with local paths, MSVC environment, and dev presets that inherit from `ninja`. See `CMakeUserPresets.json.template` for the template.

```bash
cmake --preset ninja-dev                    # Configure (once)
cmake --build --preset release-dev          # Build (RelWithDebInfo)
cmake --build --preset debug-dev            # Build (Debug)
ctest --preset test-release-dev             # Test (RelWithDebInfo)
ctest --preset test-debug-dev               # Test (Debug)
```

### MSBuild (Windows)

```bash
nuget restore .
msbuild /m /p:Configuration=Release .
```

## CI

GitHub Actions builds on every push/PR to `release/2.5`:

| Platform | Compiler | Architecture |
|----------|----------|--------------|
| Windows | MSVC | x64 |
| Windows | MSVC | x86 |
| Ubuntu | GCC | x64 |
| Ubuntu | Clang | x64 |

Dependency repos (`scada-core`, `scada-common`, `transport`, etc.) are checked out automatically. Modules requiring proprietary SDKs (Modus, Classic OPC, Vidicon) are disabled in CI.

## Project Structure

```
scada-client/
├── app/                # Application entry points (qt/ and wt/ subdirs)
├── aui/                # Abstract UI layer (platform-agnostic models)
├── base/               # Foundation utilities
├── clipboard/          # Clipboard and node serialization
├── components/         # ~22 reusable UI components
├── configuration/      # Device/node configuration management
├── controller/         # MVC controller layer and command registry
├── core/               # Core module: command registries, tracer, progress
├── events/             # Event system, journal, local events
├── export/             # CSV and configuration export/import
├── favorites/          # Bookmarks management
├── filesystem/         # File system operations and caching
├── graph/              # Graph/chart visualization
├── main_window/        # Main window management and page lifecycle
├── modus/              # Modus 6.30 ActiveX/COM integration (Qt only)
├── portfolio/          # Portfolio management
├── print/              # Print and print preview
├── profile/            # User profiles, window definitions, layouts
├── properties/         # Property management and dialogs
├── services/           # Shared services (speech, tasks, telemetry)
├── timed_data/         # Time-series data service
├── vidicon/            # Vidicon protocol integration (Qt only)
├── web/                # Web component
├── res/                # Resources and settings
└── test/               # Integration tests
```

Modules with UI code contain `qt/` and `wt/` subdirectories for platform-specific implementations. The custom `client_module.cmake` build system creates dual targets (`<name>_qt` and `<name>_wt`) automatically for each module.

## Architecture

The application uses a modular MVC architecture with context-based dependency injection. Each module defines a `*Context` struct containing its dependencies and privately inherits from it:

```
main() -> AppInit -> ClientApplication -> [CoreModule, EventModule, MainWindowModule, ...]
```

Three pluggable data service backends are supported via the `REGISTER_DATA_SERVICES` macro:

| Backend | Protocol | Default Address |
|---------|----------|-----------------|
| Scada | Telecontrol | `localhost` |
| OPC UA | OPC UA | `opc.tcp://localhost:4840` |
| Vidicon | Vidicon | `localhost` |

## Command-Line Switches

| Switch | Description |
|--------|-------------|
| `--verbose-logging` | Enable verbose log output |
| `--log-service-read` | Log service read operations |
| `--log-service-browse` | Log service browse operations |
| `--log-service-history` | Log service history operations |
| `--log-service-event` | Log service events |
| `--log-service-model-change-event` | Log model change events |
| `--log-service-node-semantics-change-event` | Log node semantics change events |

## Discovery

http://telecontrol.ru/discovery.json

## Telemetry

Add to the `discovery.json` when possible:

```json
"telemetry": "https://d26i7akorx31n9.cloudfront.net/telemetry",
```

## Updates

### Use cases

* Check for updates once per hour.
* Once an update is detected, register a local event.
* An option to stop all update checks.
* A command to download the update.

### Schema

```json
{
  "scada": {
    "versions": {
      "2.3.8": {
        "description": "Many updates",
        "installer": "https://telecontrol-public.s3-us-west-2.amazonaws.com/telecontrol-scada/telecontrol-scada-2.3.8.msi"
      }
    }
  }
}
```

## Screenshot Generator

The screenshot generator (`app/screenshot_generator.cpp`) captures PNG screenshots of
client window types using an offscreen Qt renderer. It is built as a GTest fixture inside
`client_qt_unittests`.

### Running

```bash
# Capture all screenshots to the default ./screenshots/ directory:
client_qt_unittests --gtest_filter="ScreenshotGenerator.*"

# Specify a custom output directory:
client_qt_unittests --gtest_filter="ScreenshotGenerator.*" --screenshot-dir=path/to/output
```

### Available tests

| Test               | Output                                                                        |
|--------------------|-------------------------------------------------------------------------------|
| `CaptureAllWindows` | Individual PNGs for each window type (graph.png, table.png, events.png, etc.) |
| `CaptureMainWindow` | `client-window.png` — composite main window with Graph, Nodes, and Events    |

### Window types captured

Graph, Table, Summary, Events, EventJournal, DeviceWatch, ObjectTree, Devices,
Users, Parameters, Sheet, Favorites, Files, TimedData, Retransmission.

## License

Apache 2.0 — see [LICENSE](LICENSE).
