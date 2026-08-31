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

## Prerequisites

- C++17 compiler (MSVC, GCC, or Clang)
- CMake 3.x+
- [vcpkg](https://vcpkg.io/) (recommended for dependency management)
- Qt 6 (Widgets, LinguistTools, PrintSupport; ActiveQt on Windows)
- Boost (ASIO, Beast, Signals2, Locale, Range, Algorithm)
- Google Test
- OPC UA SDK (the `opcuapp` product, `third_party/opcuapp`)
- Windows SDK / ATL (Windows only, for Modus and COM support)

## Building

### CMake Presets (Recommended)

```bash
cmake --preset ninja                 # Configure
cmake --build --preset release       # Build (or: debug, relwithdebinfo)
ctest --preset test-release          # Test (or: test-debug)
```

Every product in the SCADA tree carries this same preset set (ADR 0011), so the
commands do not change from one to the next. Set `VCPKG_ROOT` in the
environment; everything else machine-specific — toolchain paths, ccache,
cppcheck, and on Windows the MSVC include/lib directories — lives in one
`.scada-local.cmake` beside `build-support/`, shared by every product. There is
no per-repo `CMakeUserPresets.json` any more.

The client consumes five products — `common`, `core`, `opcuapp`, `graph_qt` and
`view_manager_qt`. In a standalone checkout they sit beside it and the resolver
in `build-support/` finds them there.

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
├── app/                # Application entry point (qt/ subdir)
├── aui/                # Abstract UI layer (platform-agnostic models)
├── base/               # Foundation utilities
├── clipboard/          # Clipboard and node serialization
├── modules/         # ~22 reusable UI components
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

Modules with UI code keep their toolkit-specific implementations in a `qt/` subdirectory. The custom `client_module.cmake` build system creates the `<name>_qt` target automatically for each module.

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

The screenshot generator captures PNG screenshots of client window types using
an offscreen Qt renderer. It lives in [`tools/screenshot_generator/`](tools/screenshot_generator)
and builds as its **own executable**, `client_screenshot_generator` — it is not
part of `client_qt_unittests`, though it still uses GTest to drive the captures,
so `--gtest_filter` selects among them.

### Running

```bash
cmake --build --preset release --target client_screenshot_generator
cd build/ninja/bin/Release
QT_QPA_PLATFORM=offscreen ./client_screenshot_generator --out=path/to/output
```

`--out` is required. `QT_QPA_PLATFORM=offscreen` is not optional on a headless
host. Nothing rebuilds the generator for you, so build it before reading any
change in its output as a regression.

| Option | Meaning |
|---|---|
| `--out=<dir>` | Output directory (required) |
| `--image-manifest=<path>` | Path to `screenshots/image_manifest.json` |
| `--data=<path>` | Fixture to drive, instead of the source tree's `screenshot_data.json` |
| `--only=<names>` | Comma/semicolon/newline-separated filenames to capture |
| `--theme=<name>` | Render under a design-token theme: `dark`, `light` or `hc` |

An unrecognised option is rejected rather than ignored — everything but
`--gtest_*` must be one of the above.

### Available captures

`--gtest_filter` selects among the `ScreenshotGenerator.*` captures; the two
broadest are:

| Test               | Output                                                                        |
|--------------------|-------------------------------------------------------------------------------|
| `CaptureAllWindows` | Individual PNGs for each window type (graph.png, table.png, events.png, etc.) |
| `CaptureMainWindow` | `client-window.png` — composite main window with Graph, Nodes, and Events    |

### Window types captured

Graph, Table, Summary, Events, EventJournal, DeviceWatch, ObjectTree, Devices,
Users, Parameters, Sheet, Favorites, Files, TimedData, Retransmission.

## License

Apache 2.0 — see [LICENSE](LICENSE).
