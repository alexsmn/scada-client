# Telecontrol SCADA Client — High-Level Design

> Status: living document. Captures the *current* shape of the client at the
> point of writing, not a forward-looking roadmap. The roadmap lives in
> `client/tasks.md`.

## 1. Purpose and Scope

The Telecontrol SCADA Client is the operator-facing application for the
Telecontrol industrial control system. It connects to a SCADA Server (or
to OPC UA / Vidicon back-ends) over the network, browses the device address
space, monitors live values, displays historical data, journals events, and
issues control commands.

This document describes:

- **Use cases** the client supports today (§2)
- **Functional requirements** that follow from those use cases (§3)
- **Non-functional requirements** baked into the architecture (§4)
- **High-level components** and how they fit together (§5)
- **Cross-cutting concerns** (§6)
- **Known limitations and open questions** (§7)

It does *not* describe individual screens, command-line switches, or build
instructions — those live in `README.md`, `command-line.md`, and the user
documentation site at <https://telecontrol-ru.github.io/scada/>.

## 2. Use Cases

The client targets three actor archetypes — **operator** (read-mostly,
monitoring shifts), **engineer** (configures devices, exports data), and
**administrator** (manages users, system settings). The use cases below are
grouped into four areas — *monitoring & visualization*, *operator tools*,
*configuration*, and *administration* — and each is grounded in concrete
components in the source tree.

<p align="center">
  <img src="use-cases.svg" alt="Use case diagram" width="820">
</p>

> Source: [`use-cases.mmd`](use-cases.mmd). Regenerate with
> `mmdc -i use-cases.mmd -o use-cases.svg -b transparent`.

The table that follows expands each use case with the source folder
that implements it. The columns are read as: identifier, actor, goal,
implementation.

| # | Actor | Use case | Source |
|---|---|---|---|
| UC-1 | Operator | Monitor live values from devices on the configured object tree, with periodic refresh and quality/limit indicators. | `components/watch/`, `components/timed_data/` |
| UC-2 | Operator | Visualise multiple time-series on a single chart with configurable time range, panes, colours, dots/steps, and legends. | `graph/`, `components/timed_data/` |
| UC-3 | Operator | View tabular real-time and historical data with sorting, filtering, and limit highlighting. | `components/table/`, `components/sheet/`, `components/summary/` |
| UC-4 | Operator | Receive and acknowledge events / alarms, with auto-flash, sound, and severity-based colour. | `events/`, `components/watch/` |
| UC-5 | Operator | Browse historical event journals over a chosen time range, filter by severity / item / area. | `events/event_module.cpp` (`EventJournal` window) |
| UC-6 | Operator | Watch a custom user-defined spreadsheet of cells bound to live values. | `components/sheet/` |
| UC-7 | Operator | Issue control commands (write set-points, switch states) with optional two-stage confirmation. | `components/write/` |
| UC-8 | Operator | Track favourite nodes for fast navigation, organise them into portfolios. | `favorites/`, `portfolio/` |
| UC-9 | Operator | Print or export the contents of the active view (table, graph, summary, journal). | `print/`, `export/csv/` |
| UC-10 | Operator | Locate, transfer and view files exchanged with the server (file system view, file cache). | `filesystem/`, `components/web/` |
| UC-11 | Operator | View Modus 6.30 schematics with click-to-navigate hot-spots overlaid with live values. *(Qt / Windows only)* | `modus/`, `vidicon/display/` |
| UC-12 | Engineer | Browse the device hardware tree, edit per-device parameters, set limits and aliases. | `configuration/devices/`, `components/limits/`, `components/node_properties/` |
| UC-13 | Engineer | Create and delete data items in bulk via multi-create dialogs and the table editor. | `components/multi_create/`, `components/node_table/` |
| UC-14 | Engineer | Export a configuration snapshot to disk, edit it externally, and import it back. | `export/configuration/` |
| UC-15 | Engineer | Inspect raw protocol traffic for a connected device (request / response inspector). | `components/debugger/`, `components/device_metrics/` |
| UC-16 | Engineer | Save the current window layout as a personal profile and restore it on next launch. | `profile/`, `main_window/` |
| UC-17 | Administrator | Authenticate with username / password against a chosen back-end (Scada, OPC UA, Vidicon). | `components/login/`, `app/client_application.cpp` |
| UC-18 | Administrator | Manage users, change passwords, set access rights. | `components/change_password/`, `components/node_table/` (`Users` window) |
| UC-19 | Administrator | Configure data transmission rules between sources and destinations. | `components/transmission/` |

The client also exposes a few cross-cutting capabilities that are not user
goals on their own but are worth listing as separate use cases because they
have explicit modules behind them:

| # | Use case | Source |
|---|---|---|
| UC-20 | Operate the same UI in either a Qt 5 desktop window **or** a Wt web page from a browser, against the same back-end. | `app/qt/`, `app/wt/`, `aui/` |
| UC-21 | Operate the UI in Russian (currently the only shipped translation). | `aui/qt/translation_qt.cpp`, `app/qt/client_ru.ts`, etc. |

## 3. Functional Requirements

Numbered for traceability. Each requirement cites the module that satisfies
it today.

### Data plane

- **FR-1.** Speak to **at least three pluggable back-ends**: native Telecontrol/SCADA, OPC UA, and Vidicon. Each back-end supplies the same set of services (attribute, monitored item, view/browse, history, session, method, node management). — `app/client_application.cpp:77-90` (`REGISTER_DATA_SERVICES`).
- **FR-2.** Browse the OPC-UA-style address space, expose hierarchical node trees with lazy fetching and progress reporting. — `configuration/`, `node_service_progress_tracker.{h,cpp}`.
- **FR-3.** Read attribute values either on demand or via long-lived subscriptions (monitored items). — `components/watch/`, `aui/models/`.
- **FR-4.** Read historical samples for any node over a user-chosen time range, with cancellation support. — `components/timed_data/`, history service via `master_data_services.h`.
- **FR-5.** Write values back to nodes (control commands, set-points, manual entry) with optional two-stage confirmation. — `components/write/`.
- **FR-6.** Receive event/alarm streams, store them in a journal, allow filtering, and acknowledge. — `events/event_module.cpp`, `events/event_fetcher.h`.
- **FR-7.** Detect connection loss and reconnect with backoff, surface connection state to the UI. — `services/connection_state_reporter.{h,cpp}`.

### Presentation

- **FR-8.** Render a configurable **multi-window, multi-page** layout. A user has one or more main windows, each with one or more named pages, each containing one or more docked or tabbed views. — `main_window/`, `profile/page.h`, `profile/window_definition.h`.
- **FR-9.** Provide ~20 reusable **view types**: graph, table, summary, sheet, watch, event journal, file system, debugger, device metrics, parameters, transmission, write, table editor, users, formats, simulation signals, historical databases, web, and a few others. Each is a `components/<name>/` module that registers a `WindowInfo` and a controller factory. — `components/`.
- **FR-10.** Persist user **profile** (window layouts, page definitions, favourites, colour preferences, alarm settings) to disk and restore it on next launch. — `profile/profile.{h,cpp}`.
- **FR-11.** Bookmark frequently used nodes and group them into named **portfolios**. — `favorites/`, `portfolio/`.
- **FR-12.** Print the active view (table, graph, summary, …) with print preview. — `print/`.
- **FR-13.** Export tabular data to **CSV** and the configuration tree to a **portable file format** that can be re-imported. — `export/csv/`, `export/configuration/`.

### Authentication and access control

- **FR-14.** Prompt the user for credentials at startup (or on disconnect), pass them to the chosen back-end, and store the resulting session. — `components/login/`, `OnLoginCompleted` in `client_application.cpp`.
- **FR-15.** Support **password change** through the UI. — `components/change_password/`.
- **FR-16.** Gate views and commands by user permission. Window definitions carry a `WIN_REQUIRES_ADMIN` flag that hides them from non-admins. — `controller/window_info.h`.

### Localization and theming

- **FR-17.** Render the entire UI in **Russian**. The translation lookup goes through a single `Translate()` shim that uses the empty Qt translation context to match `lupdate`-generated `.ts` files. — `aui/qt/translation_qt.cpp`, `app/qt/client_ru.ts`, `main_window/qt/main_window_ru.ts`.
- **FR-18.** Default to a consistent visual style (Fusion Qt style) with configurable alarm and bad-value colours stored in the profile. — `app/qt/installed_style.h`, `profile/profile.h`.

### Platform-specific integrations

- **FR-19.** Embed **Modus 6.30** schematics via ActiveX, exposing live values on top of static drawings, with click-to-navigate. *Qt + Windows only.* — `modus/libmodus/`, `modus/activex/`.
- **FR-20.** Embed the **Vidicon** display protocol for sites running mixed Telecontrol + Vidicon stacks. *Qt + Windows only.* — `vidicon/display/`, `vidicon/teleclient/`.

### Tooling and operations

- **FR-21.** Expose an offline **screenshot generator** that loads a JSON fixture and renders every window type to PNG, used both for documentation and for visual regression testing. — `app/screenshot_generator.cpp`.
- **FR-22.** Emit metrics and traces about itself for centralised observability. — `core/core_module.h` (`Tracer`), `metrics/boost_log_metric_reporter.h`.
- **FR-23.** Read run-time options from the command line (verbose logging, per-service logging toggles, locale override). — `command-line.md`, `base/program_options.{h,cpp}`.

## 4. Non-Functional Requirements

- **NFR-1. Modularity by dependency injection.** Every subsystem follows the *Context struct + private inheritance* pattern (`Module(ModuleContext&&)`). Modules are constructed in `ClientApplication` with explicit dependencies; there are no hidden globals beyond the registered backends and command registries. *(See CLAUDE.md §"Module-Based MVC".)*
- **NFR-2. Async-first I/O.** All I/O calls return `promise<T>` (from scada-core's `promise.hpp`). Continuations are pinned to the originating executor with `BindPromiseExecutor` so UI thread affinity is preserved. — used pervasively; see `client_application.cpp` start sequence.
- **NFR-3. Dual UI on a shared model layer.** The application is built twice: once linked against Qt 5 (`*_qt` targets, `app/qt/main.cpp`) and once against Wt (`*_wt` targets, `app/wt/main.cpp`). Models live in module roots and are platform-agnostic; Qt-specific implementations live in `qt/` subdirs, Wt-specific in `wt/`. The `UI_WT` macro guards Modus and Vidicon, which are Qt-only. — `client_module.cmake`.
- **NFR-4. Cross-platform builds.** Windows (MSVC) and Linux (GCC, Clang) are first-class CI targets via the CMake presets. Dependencies are managed by vcpkg. — `vcpkg.json`, `.github/workflows/cmake-multi-platform.yml`.
- **NFR-5. Testability without a server.** Each module ships with `*_unittest.cpp` files using GoogleTest; tests use `TestExecutor` for deterministic async, and a set of in-memory local services (`scada::Local{Attribute,View,History,…}Service`) lets the screenshot generator and tests run with no real network. — `aui/test/`, `common/address_space/local_*` in scada-common.
- **NFR-6. Resilient connections.** Loss of the back-end is recovered with exponential-backoff reconnect rather than aborting; the UI keeps running and surfaces connection state to the user. — `services/connection_state_reporter.{h,cpp}`.
- **NFR-7. Bounded destruction order.** `ClientApplication`'s destructor releases modules in a specific order to respect the dependency graph; this is documented as a maintenance constraint. *(CLAUDE.md §"Destruction order matters".)*
- **NFR-8. Observability built in.** A `Tracer` instance is created in the core module before anything else and threaded through every operation; metrics are reported via `BoostLogMetricReporter` on a recurring schedule. — `core/core_module.h`, `metrics/`.
- **NFR-9. Localisation-aware.** All user-visible strings flow through `Translate()` (or `tr()` in `.ui` files) so the same binary supports any locale that has a `client_<lang>.qm` shipped next to the executable.
- **NFR-10. Backwards-compatible profile format.** Profiles are JSON, written through `profile/window_definition_util.cpp` with explicit field names so older profiles keep loading after schema additions.
- **NFR-11. UTF-8 source files where Cyrillic appears.** C++ files containing Cyrillic `u"..."` literals must carry a UTF-8 BOM so MSVC parses them under UTF-8 rather than the system code page. *(See `feedback_translation_bug.md` and the recent mojibake repair commits.)*

## 5. High-Level Components

The client decomposes into roughly the following layers. Each layer is a
small set of source directories with a single coherent purpose; layers
generally depend downward.

<p align="center">
  <img src="architecture-layers.svg" alt="Architecture layers" width="720">
</p>

> Source: [`architecture-layers.mmd`](architecture-layers.mmd). Regenerate
> with `mmdc -i architecture-layers.mmd -o architecture-layers.svg -b transparent`.

### 5.1 Application bootstrap — `app/`

`ClientApplication` is the top-level orchestrator. Its constructor accepts a
`ClientApplicationContext` (executor, login handler, optional service
overrides for tests) and instantiates every module in dependency order.
`Start()` returns a `promise<void>` that resolves when login completes and
the first profile page is displayed.

Two `main()` entry points exist:

- `app/qt/main.cpp` builds `client.exe`, the Qt 5 desktop client.
- `app/wt/main.cpp` builds the Wt web server that serves the same UI over
  HTTP.

Both entry points instantiate the same `ClientApplication`; the difference
is which executor/message-loop they hand it.

Also lives here: `app_init.{h,cpp}` (one-time GDI+ / ATL setup),
`screenshot_generator.cpp` (offline rendering harness for docs and tests),
and `client_application_unittest.cpp`.

The full startup sequence — from `main()` through login to the first
visible page — looks like this:

<p align="center">
  <img src="bootstrap-sequence.svg" alt="Bootstrap sequence" width="820">
</p>

> Source: [`bootstrap-sequence.mmd`](bootstrap-sequence.mmd). Regenerate
> with `mmdc -i bootstrap-sequence.mmd -o bootstrap-sequence.svg -b transparent`.

### 5.2 Core orchestration — `core/`

Owns the singletons every other module needs: `Tracer` for observability,
`GlobalCommandRegistry` (main menu / toolbar), `SelectionCommandRegistry`
(context menu), and `ProgressHost` for showing async-task progress in the
UI.

### 5.3 Abstract UI layer — `aui/`

Qt-and-Wt-agnostic models and helpers. Tables, trees, grids, and the
`Translate()` shim live here. Platform-specific implementations live in
`aui/qt/` and `aui/wt/`. The `aui/test/` subdir provides `AppEnvironment`,
which is the per-test fixture for spinning up a bare `QApplication` (or
`WServer`).

### 5.4 Services — `services/` and `common/master_data_services`

Cross-cutting per-session services:

- `TaskManager` — queues async operations, reports progress.
- `ConnectionStateReporter` — polls the back-end, retries with backoff.
- `TimedDataService` — combines real-time and historical data behind one
  interface, used by graph/summary/timed-data views.
- `SpeechService` — text-to-speech for alarm notifications.
- `MetricService` (via `metrics/`) — collects per-operation latencies.

The actual data-service interfaces (`AttributeService`, `MonitoredItemService`,
`ViewService`, `HistoryService`, `SessionService`, `MethodService`,
`NodeManagementService`) come from `common/master_data_services.h` in
scada-common, and are wired up at login time from one of the three
registered back-ends:

- **Scada (Telecontrol)** — default; talks to a Telecontrol Server.
- **OPC UA** — `opc.tcp://localhost:4840` by default.
- **Vidicon** — Vidicon-protocol endpoint.

Backends are registered statically via the `REGISTER_DATA_SERVICES` macro in
`client_application.cpp`.

### 5.5 Profile and persistence — `profile/`

`Profile` is the durable representation of "what this user sees on launch":
window bounds and state, page list, page contents (`WindowDefinition`s),
favourites, alarm colours, sound preferences, default time ranges,
`bad_value_color`, etc. It is loaded from JSON on startup and rewritten on
shutdown via `Profile::Save()`.

### 5.6 Domain modules

Each is a thin module that registers its windows, controllers, and commands
with the central registries. There are 12 of them today:

| Module | Source | Responsibility |
|---|---|---|
| `CoreModule` | `core/` | Registries, tracer, progress host (§5.2). |
| `MainWindowModule` | `main_window/` | Window/page lifecycle (§5.7). |
| `ConfigurationModule` | `configuration/` | Object tree, hardware tree, raw nodes view. |
| `EventModule` | `events/` | Event fetching, journaling, local error events. |
| `ExportConfigurationModule` | `export/configuration/` | Configuration snapshot export/import. |
| `CsvExportModule` | `export/csv/` | CSV export for tabular views. |
| `FavoritesModule` | `favorites/` | Favourite nodes. |
| `PortfolioModule` | `portfolio/` | Named groupings of favourites. |
| `DebuggerModule` | `components/debugger/` | Protocol-level request/response inspector. |
| `ModusModule` *(Qt only)* | `modus/` | Modus 6.30 ActiveX schematic embedding. |
| `VidiconModule` *(Qt only)* | `vidicon/` | Vidicon display embedding. |
| `WebModule` | `components/web/` | Embedded web view component. |

Modules' `*Context` structs make their dependencies explicit; `ClientApplication`
constructs them in an order that satisfies the graph:

<p align="center">
  <img src="module-graph.svg" alt="Module graph" width="820">
</p>

> Source: [`module-graph.mmd`](module-graph.mmd). Regenerate with
> `mmdc -i module-graph.mmd -o module-graph.svg -b transparent`.

### 5.7 Main window and view manager — `main_window/`

A `MainWindowManager` owns one or more `MainWindow` instances. Each main
window contains a stack of `Page` objects from the profile, and each page
hosts one or more `OpenedView`s laid out as docks and tab groups.

Important responsibilities:

- `OpenedView::GetWindowTitle()` runs the English `WindowInfo::title`
  through `Translate()` so localised builds resolve to the `.ts`
  translation.
- `EventDispatcher` drives the auto-show / auto-hide / flash / sound
  behaviour for incoming alarms.
- `view_manager_qt.cpp` / `view_manager_wt.cpp` adapt the platform-agnostic
  layout to Qt docks vs Wt panels.

### 5.8 Controllers and commands — `controller/`

`ControllerRegistry` maps a `command_id` to a `ControllerFactory`. New
windows are opened by looking up the controller for their `WindowInfo`
and invoking it with a `ControllerContext`. Static registration uses
`REGISTER_CONTROLLER(MyController, kFooWindowInfo)`; dynamic registration
goes through `ControllerRegistry::AddControllerFactory` and is used by
modules whose factories close over services from their context (see
`ConfigurationModule`).

Action commands (main menu, toolbar, page commands, selection commands)
are registered into `GlobalCommandRegistry` and `SelectionCommandRegistry`
from `core/`.

### 5.9 UI components — `components/`

Twenty self-contained component subdirectories, each describing a single
view type or dialog. Most define a `WindowInfo`, register a controller,
and split into `qt/` and `wt/` subdirs:

| Component | Purpose |
|---|---|
| `about/` | About-the-application dialog. |
| `change_password/` | Change-password dialog. |
| `create_service_item/` | Wizard for creating a single service node. |
| `debugger/` | Protocol request/response inspector. |
| `device_metrics/` | Per-device performance counters. |
| `limits/` | Edit alarm/warning thresholds for an analog item. |
| `login/` | Authentication dialog. |
| `multi_create/` | Bulk-create wizard for many nodes at once. |
| `node_properties/` | Properties grid for any selected node. |
| `node_table/` | Generic editable grid (Users, Formats, Simulation Signals, Historical Databases). |
| `select_item/` | Modal node picker. |
| `sheet/` | Spreadsheet-like view of cells bound to live values. |
| `summary/` | Aggregate (count / min / max / avg) summary view. |
| `table/` | Generic real-time + historical table view. |
| `time_range/` | Time-range picker dialog. |
| `timed_data/` | Time-series view (used by graph and tables). |
| `transmission/` | Re-transmission rules editor. |
| `watch/` | Live "watch list" of monitored values. |
| `web/` | Embedded web view. |
| `write/` | Write-value / control-command dialog. |

Other view-bearing modules (Graph, Event Journal, Configuration trees,
Favourites, File system, Modus, Vidicon) live outside `components/` because
they predate the components convention or are too large to fit it; the
plan in `tasks.md` is to keep extracting them.

### 5.10 Platform integrations — `modus/`, `vidicon/`

Both are Qt-only and Windows-only (`#if !defined(UI_WT)`).

- `modus/` wraps the Modus 6.30 ActiveX control. The Russian-language
  OLESTR parameter names (`"ключ_привязки"`, `"положение"`, `"уставки"`)
  are part of the external protocol and **must not be renamed**.
- `vidicon/` embeds the Vidicon display protocol via a `VidiconClient`
  that bridges Telecontrol's `TimedDataService` to the Vidicon side.

### 5.11 Filesystem, export, print, graph

- `filesystem/` — async file I/O wrapper with a local cache, used for
  uploads/downloads against the server-side file system.
- `export/configuration/` — configuration tree snapshot exporter and
  importer (binary format).
- `export/csv/` — CSV writer used by tables, summaries, and journals.
- `print/` — print preview and printing for any view that exposes a
  printable model.
- `graph/` — multi-pane time-series chart, built on top of the external
  `graph-qt` package.

## 6. Cross-Cutting Concerns

- **Dependency injection.** Every module takes a `*Context&&` and
  privately inherits from it; no global state beyond the static command
  and data-service registries.
- **Async / promise-based control flow.** Every I/O returns `promise<T>`
  from scada-core. The bootstrap chain in `ClientApplication::Start()` is
  itself a `.then()` ladder.
- **Localisation.** A single `Translate()` function (`aui/qt/translation_qt.cpp`)
  routes through `QCoreApplication::translate("", text)` to match the
  empty translation context that `lupdate` writes into the `.ts` files.
  English literals in code, Russian (and any future languages) in `.ts`.
- **Theming.** Default Qt style is Fusion (set in `app/qt/main.cpp`).
  Profile carries `bad_value_color`, `alarm_color`, and similar palette
  hints used by the views.
- **Error handling.** Background failures publish to `LocalEvents`, which
  surfaces them in the events panel as red entries; fatal errors throw
  exceptions that propagate up the promise chain to the executor and end
  up in the boost.log sink.
- **Permissions.** `WIN_REQUIRES_ADMIN` on `WindowInfo` is the front-line
  check; back-end services enforce again. The `change_password/` and
  `node_table/` (Users) components are the admin surface area.
- **Observability.** `Tracer` is created first by `CoreModule` and
  threaded through every other module; `MetricService` collects timings;
  `BoostLogMetricReporter` flushes them to the log on a periodic timer.

## 7. Known Limitations and Open Questions

These are observations worth flagging in any future maintenance or
roadmap discussion. They are *not* prescriptive — many are intentional.

- **Wt feature parity is partial.** Modus, Vidicon, and the Qt-only paths
  in some components are unavailable in the Wt build. There is no
  parity test that ensures both UIs render the same fixture.
- **Configuration export format is binary and undocumented in this repo.**
  The exact wire format is implicit in `export/configuration/`. A schema
  document or version negotiation would reduce the migration risk if the
  format changes.
- **OPC UA back-end depends on an external SDK.** `third_party/opc/` is
  pulled from outside this repo; an SDK upgrade is a coordinated effort.
- **No formal performance budget.** `TimedDataService` and the historical
  views can in principle load tens of thousands of points; there is no
  documented upper bound or a regression guard for either query latency
  or memory.
- **Translation coverage is one language.** Only `client_ru.ts` ships.
  Adding another language is a matter of running `lupdate` and shipping
  the new `.qm`, but the workflow is not documented end-to-end.
- **`promise<T>` and executor pinning are subtle.** Forgetting
  `BindPromiseExecutor` on a continuation can move work to the wrong
  thread without an obvious symptom. The `tasks.md` entry on dependency
  injection hints at moving toward a more declarative style.
- **A handful of view-bearing modules predate the `components/`
  convention** (Graph, Event Journal, Configuration trees, Favourites,
  File system) and live at the top level. Their layout is consistent but
  inhomogeneous with the components folder.
- **Mojibake remains a hazard for files with Cyrillic literals**
  whenever a tool round-trips them through UTF-8 without a BOM. The
  pattern of "English in source, Russian in `.ts`" should be the
  default for new code; existing files have been repaired but the
  underlying tooling risk remains.

---

*Source as of writing: client repo at the head of `main`; this document
should be updated alongside any change that adds a top-level module, a
new domain folder under `client/`, or a new data-service back-end.*
