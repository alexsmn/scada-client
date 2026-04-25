# Client Coroutine Migration Plan

## Summary

The client currently uses `promise<T>`-based async control flow across startup,
service calls, file I/O, and UI-triggered background work. The migration should
move client internals to coroutine-first implementations while preserving the
existing UI-facing threading guarantees and avoiding a repo-wide API break in
the first phase.

The recommended approach is:

1. Migrate client implementations and workflows to coroutine-native internals,
   and switch client async code to the coroutine versions of the core SCADA
   services.
2. Migrate internal client workflows with the highest callback/promise nesting.
3. Keep public service contracts and module wiring stable with temporary
   adapters at legacy boundaries until the migrated paths are proven in tests.

## Current Constraints

- UI work must stay affinity-safe for both Qt and Wt frontends.
- Most client async flows still assume `promise<T>` and
  `BindPromiseExecutor`-style continuation pinning.
- Client tests and screenshot tooling rely on deterministic local services and
  must keep working without a real server.
- Some client test targets are already memory-heavy to compile, so migration
  should avoid template-heavy abstractions unless they remove equivalent
  promise/callback boilerplate.

## Goals

- Reduce nested `.then()` chains and callback wrapping in client workflows.
- Standardize coroutine usage at client/core boundaries without turning
  per-class adapters into a permanent architecture.
- Preserve executor/thread affinity for UI-facing continuations.
- Keep startup, shutdown, cancellation, and task-progress behavior unchanged
  from the user's perspective.
- Keep the first migration phase compatible with existing tests and local mock
  services.

## Non-Goals

- A full repo-wide replacement of `promise<T>` in one pass.
- Rewriting all services or all UI modules at once.
- Changing the frontend split between Qt and Wt.
- Changing wire protocols, authentication flow, or local-service semantics.
- Migrating code that fundamentally requires multi-wait semantics to coroutines.

## Phase 1: Shared Adapter Layer

Add coroutine support without breaking current public client interfaces.

### Work

- Reuse the shared adapter utilities introduced in `core/base` and
  `core/scada`:
  - promise-to-awaitable bridging
  - callback-to-awaitable bridging
  - named coroutine service adapters in
    `core/scada/coroutine_services.h`
- Define the default client-side rule for core SCADA service calls:
  - coroutine code should depend on `Coroutine*Service` interfaces or their
    adapters
  - do not add new client coroutine code that calls callback-style
    `AttributeService`, `MethodService`, `HistoryService`, `ViewService`, or
    `NodeManagementService` directly
  - do not add new client coroutine code that awaits raw `SessionService`
    promises directly when `PromiseToCoroutineSessionServiceAdapter` can be
    used instead
- Prefer these boundary conversions:
  - callback core service -> `CallbackToCoroutine*ServiceAdapter`
  - promise core service -> `PromiseToCoroutineSessionServiceAdapter`
  - coroutine implementation exposed back to legacy code ->
    `CoroutineToCallback*ServiceAdapter` or
    `CoroutineToPromiseSessionServiceAdapter`
- Prefer migration over adapter proliferation:
  - when touching a client class for coroutine work, first prefer converting
    that class's internal control flow to coroutines
  - add adapters only at legacy boundaries that cannot be migrated in the same
    slice
  - treat those adapters as temporary compatibility shims, not new long-term
    layering for every class
- Add client-focused guidance for executor usage:
  - UI-bound coroutine continuations must resume on the original executor
  - background work may hop to worker executors only through explicit adapters
- Standardize one cancellation rule:
  - module/application lifetime owns coroutine lifetime
  - closing the app, page, or session cancels outstanding work before teardown

### Acceptance Criteria

- New coroutine code uses the coroutine versions of core SCADA services rather
  than inline callback wrapping or ad hoc promise bridging.
- Client modules can await core SCADA service calls through
  `Coroutine*Service` interfaces or the named adapters from
  `core/scada/coroutine_services.h`.
- Existing `promise<T>`-returning client entry points can be implemented via
  coroutine internals.
- No observable UI thread-affinity regressions.

## Phase 2: Startup And Session Flow

Migrate the startup path first because it is central, heavily async, and easy
to regression-test.

### Target Areas

- `client/application` startup sequence
- login/session establishment
- initial model population after login
- shutdown and canceled-login flows

### Work

- Rewrite internal `Start()` orchestration as coroutine steps:
  - load profile
  - show/login resolve
  - create session through the coroutine session service adapter
  - initialize modules/models
  - surface startup errors centrally
- Keep outward `promise<void>` or equivalent entry points unchanged in phase 2.
- Replace deeply chained `.then()` trees with `co_await` sequencing.
- When bridging promise-based session lifecycle APIs into coroutine internals,
  ensure reconnect/shutdown flows wait on each completion boundary exactly
  once.
  In particular, do not combine a disconnect promise that already waits for a
  connect-loop shutdown signal with a second explicit wait on that same signal.

### Status

- **Qt application startup chain** migrated
  (`client/app/qt/main.cpp`, `client/app/qt/startup_flow.{h,cpp}`). The
  posted `app.Start().then(...).then(...).except(...)` tree is now a
  coroutine-backed `RunQtStartupFlow(...)` helper that awaits startup, E2E
  checks, steady-state `Run()`, and startup failure reporting in one linear
  sequence. The helper preserves the public `promise<void>` boundary, reports
  E2E success/failure through the existing hooks, logs E2E run completion in
  E2E mode, and invokes `QApplication::quit` only for normal-mode completion.
  Regression coverage: `client/app/qt/startup_flow_unittest.cpp`.
- **Qt operator-use-case E2E smoke runner** migrated
  (`client/app/qt/e2e_test_support.{h,cpp}`). The long
  `promise<> sequence = sequence.then(...)` pipeline in
  `RunE2eOperatorUseCaseSmoke(...)` is now a coroutine-backed runner that
  awaits each window-open and registration-check step in order, while keeping
  the public `promise<>` entry point and report format. `OpenOperatorWindow`
  was folded into an awaitable helper that maps `MainWindow::OpenView(...)`
  results without a `.then(...)` continuation. Regression coverage:
  `client/app/qt/e2e_test_support_unittest.cpp`.
- **Qt object-view/object-tree E2E polling reports** migrated
  (`client/app/qt/e2e_test_support.{h,cpp}`). `ObjectViewValuesCheck::Run()`
  and `ObjectTreeLabelsCheck::Run()` no longer retain polling objects through
  `promise_.then(...)`; both report flows now use coroutine-backed polling
  loops with the shared `Delay(...)` helper, preserving success and timeout
  report contents. Regression coverage:
  `client/app/qt/e2e_test_support_unittest.cpp`.
- **Screenshot-generator promise wait helpers** migrated
  (`client/tools/screenshot_generator/screenshot_wait.{h,cpp}`,
  `main.cpp`, `dialog_capture.cpp`). The offline screenshot generator no
  longer bridges startup/shutdown or pending-node promises through local
  `.then(...)` callbacks. The shared `WaitForPromise(...)` helper pumps the Qt
  event loop until the promise is ready and then calls `get()`, preserving the
  previous single-threaded `MessageLoopQt` behavior while propagating rejected
  promises. `WaitForPendingNodeLoads(...)` now uses the same helper and keeps
  the existing gtest failure report for rejected pending-node waits.
  Regression coverage:
  `client/tools/screenshot_generator/screenshot_wait_unittest.cpp` via
  `client_qt_unittests`.
- **ClientApplication Qt test wait loops** migrated
  (`client/app/client_application_unittest.cpp`). The test-only helpers that
  observed `ClientApplication::{Start,Quit}` promises through local
  `.then(...)` callbacks now poll promise readiness while pumping the same
  Qt/asio event loops, then call `get()` to preserve rejection propagation.
  The delayed-login assertion now checks the returned start promise directly
  instead of attaching a completion callback. Regression coverage:
  `ClientApplicationPromiseWaitTest.*` plus the existing startup/shutdown
  application tests in `client_qt_unittests`.
- **profile/** audited and marked out of scope for coroutine migration
  (`client/profile/*`). The profile layer is synchronous persistence/model
  code and has no `promise<T>`, callback continuation, executor, or coroutine
  boundary that forces profile consumers onto legacy async APIs. No production
  coroutine slice is required here.
- **Qt modal-dialog unit-test wait helpers** consolidated
  (`client/aui/qt/dialog_test_util.h`,
  `client/aui/qt/dialog_util_unittest.cpp`,
  `client/components/time_range/qt/time_range_dialog_unittest.cpp`,
  `client/export/csv/qt/csv_export_dialog_unittest.cpp`,
  `client/properties/transport/qt/transport_dialog_unittest.cpp`). The
  repeated per-test `ProcessEventsUntilSettled(...)` loops now use one shared
  test helper for promise readiness polling and active-modal dialog actions.
  Regression coverage includes the existing accepted/rejected dialog tests plus
  `DialogUtilTest.DialogTestUtilDoesNotInvokeActionForSettledPromise`.

### Risks

- Startup currently encodes ordering implicitly in promise chains; coroutine
  rewrites must preserve that order exactly.
- Error propagation must still map to the same user-visible dialogs/messages.
- Session reconnect logic is especially sensitive to duplicate completion
  waits; a coroutine migration can accidentally create self-referential promise
  chains if old loop-completion promises are awaited both indirectly and
  explicitly.

## Phase 3: Task And Progress Infrastructure

The next migration slice should target the places where the client currently
wraps async work to report progress or serialize UI updates.

### Status

- **TaskManagerImpl** internals migrated to coroutine bodies
  (`client/services/task_manager_impl.{h,cpp}`). Each `Post*Task` is now a
  coroutine lambda that `co_await`s the callback-based core services through
  `scada::CallbackToCoroutine{Attribute,NodeManagement}ServiceAdapter`. The
  public `TaskManager` interface still returns `promise<T>` so client callers
  are unaffected. `StartTask` now `CoSpawn`s the task body; queue sequencing,
  progress-dialog timing, and `ReportRequestCompletion` semantics are
  preserved. Progress hosts / progress dialogs, command handlers, and
  long-running export/import/file operations remain on the follow-up list
  below.

### Target Areas

- `TaskManager` — internals migrated (see Status)
- progress hosts / progress dialogs
- command handlers that queue async work
- long-running export/import/file operations

### Work

- Introduce coroutine-friendly task submission helpers that:
  - accept awaitable work
  - report progress consistently
  - preserve current cancellation semantics
- Convert command handlers from promise chains to coroutine bodies.
- Remove repeated callback-to-promise glue from module actions.

### Acceptance Criteria

- Commands triggered from menus, toolbars, and dialogs can be written as
  coroutine functions with a single task-submission path.
- Progress reporting and cancellation remain consistent across Qt and Wt.

## Phase 4: Data/Service Consumers

Once the adapter layer is stable, migrate client modules that frequently call
SCADA services or local service doubles.

### Status

- **filesystem/FileManagerImpl** migrated
  (`client/filesystem/file_manager_impl.{h,cpp}`). `DownloadFileFromServer`
  and the new `GetFileNodeAsync` helper are coroutine bodies that
  `co_await AwaitPromise(executor, scada::client::...)` instead of
  chaining `.then()` four times. The public
  `promise<void> FileManager::DownloadFileFromServer(...)` surface is
  unchanged — callers, mocks, and `FileSynchronizer` continue to see a
  `promise<void>`. `FileManagerContext` now takes a
  `std::shared_ptr<Executor>`; `FileSystemComponentContext` (and the
  construction site in `ClientApplication::CreateFeatureComponents`)
  were updated to thread it through.
- **filesystem/filesystem_commands** migrated
  (`client/filesystem/filesystem_commands.{h,cpp}`).
  `OpenFileCommandImpl::Execute` and `::OpenFile` are now coroutine
  bodies (`ExecuteAsync` / `OpenFileAsync`); the free helpers
  `OpenJsonFileAsync` and the coroutine-internal `AddFileAsync` replace
  the nested `.then(...).except(...)` pipelines that ran the download
  branch, the unknown-extension dialog, the invalid-JSON dialog, and
  the "add file" flow. The public `OpenFileCommandImpl::Execute`
  surface still returns `promise<void>`, and `AddFile` still returns
  `promise<>`; they spawn the coroutine via `ToPromise`. `AddFile`
  gained an `std::shared_ptr<Executor>` parameter, which
  `FileSystemComponent` threads through from its own context. The
  unused `CreateFileDirectory` helper was removed rather than
  migrated. The dependency on `main_window/main_window_util.h::OpenView`
  was dropped; the coroutine bodies call `MainWindowInterface::OpenView`
  directly so `client_filesystem` no longer compiles-in the free
  helper. Regression coverage lives in
  `client/filesystem/filesystem_commands_unittest.cpp`.
- **events/EventView::SelectSeverity** migrated
  (`client/events/event_view.{h,cpp}`). The prompt/parse/apply pipeline
  is now a coroutine (`SelectSeverityAsync`); the public `promise<>`
  entry spawns it via `ToPromise(NetExecutorAdapter{executor_}, ...)`.
  The `ResourceError` handling path uses `ShowResourceError<void>` +
  `AwaitPromise` instead of `CatchResourceError` wrapped around
  `ParseSeverity`. No context changes were required — `EventView`
  already receives an `Executor` via `ControllerContext::executor_`.
- **events/EventModule::AddOpenCommand** migrated
  (`client/events/event_module.cpp`). The selection-command's
  `execute_handler` no longer chains
  `GetOpenWindowDefinition().then(...)`; it captures
  `EventModule::executor_` and spawns a `CoSpawn` coroutine that
  `co_await`s the `WindowDefinition` and then calls
  `MainWindowInterface::OpenView` with the optional `mode` item
  injected. Regression coverage:
  `client/events/event_module_unittest.cpp::OpenEventsCommandRoutesToMainWindowOpenView`.
- **main_window/SelectionCommands** migrated
  (`client/main_window/selection_commands.cpp`). The shared
  `MakeOpenViewCommand` helper takes an
  `std::shared_ptr<Executor>` and dispatches via `CoSpawn`/
  `AwaitPromise`. The `ID_OPEN_DEVICE_METRICS`,
  `ID_OPEN_GROUP_TABLE`, and `ID_DELETE` execute handlers, plus the
  `OpenWindow(WindowInfo*)` helper, all use `CoSpawn(executor_,
  cancelation_, ...)` (or a plain `CoSpawn` where the original lambda
  did not gate on `cancelation_`) instead of
  `cancelation_.Bind(...).then(...)`. `BindPromiseExecutor` is no
  longer needed in this file.
- **main_window/OpenedViewCommands** migrated
  (`client/main_window/opened_view_commands.{h,cpp}`).
  `CreateRecord` now returns `void` (every previous caller discarded
  the `promise<>`) and spawns a `cancelation_`-gated coroutine that
  awaits `task_manager_.PostInsertTask` once and reports success or
  failure through `ReportRequestResult` exactly once. The follow-up
  `OnCreateRecordComplete` is now an `Awaitable<void>`
  (`OnCreateRecordCompleteAsync`) that `co_await`s
  `FetchNode(...)` instead of subscribing a
  `cancelation_.Bind`-wrapped `Fetch` callback. The custom-time-range
  command also moved off `.then` to `CoSpawn(executor_, cancelation_,
  ...)`. The dependency on `OpenView(promise<WindowDefinition>, ...)`
  was dropped — `client/main_window/main_window_util.{h,cpp}` lost
  that unused promise-input overload as part of this slice.
- **main_window/main_window_util::ExecuteDefaultNodeCommand** migrated
  (`client/main_window/main_window_util.cpp`). The
  `ExpandGroupItemIds(...).then(...)` pipeline is now a `CoSpawn`
  coroutine that awaits the `NodeIdSet` promise before mutating the
  active `ContentsModel`.
- **components/device_metrics** migrated
  (`client/components/device_metrics/node_collector.{h,cpp}`,
  `client/components/device_metrics/device_metrics_command.{h,cpp}`).
  Recursive device collection now has coroutine-native
  `FetchNodeAsync`, `CollectChildrenAsync`, and
  `CollectNodesRecursiveAsync` helpers that await node fetch promises on the
  caller-provided executor. `MakeDeviceMetricsWindowDefinitionAsync`
  builds the sheet definition from those helpers, while the legacy
  `promise<WindowDefinition> MakeDeviceMetricsWindowDefinition(...)`
  remains as a thin `ToPromise` wrapper. The
  `ID_OPEN_DEVICE_METRICS` selection command now awaits the coroutine
  function directly inside its existing cancellation-gated `CoSpawn` body.
  The remaining `CollectChildren(...)` and `CollectNodesRecursive(...)`
  promise compatibility wrappers now delegate to the same coroutine traversal
  body through a thread executor instead of keeping a second recursive
  `.then(...)` implementation.
  Regression coverage:
  `client/components/device_metrics/device_metrics_command_unittest.cpp`.
- **properties + components/node_table** partially migrated
  (`client/properties/property_service.{h,cpp}`,
  `client/properties/property_util.{h,cpp}`,
  `client/properties/property_context.h`,
  `client/components/node_table/node_table_model.cpp`).
  `PropertyService` now has coroutine-native
  `GetChildPropertyDefsAsync` / `GetAllSubtypesPropertiesAsync` helpers for
  UI models that already own an executor. The legacy
  `promise<PropertyDefs> GetChildPropertyDefs(...)` compatibility entry point
  is now a thin `ToPromise` adapter over `GetChildPropertyDefsAsync`, so
  existing promise callers use the same coroutine body.
  `NodeTableModel::SetParentNode` now runs the property-def load, child fetch,
  column update, and row update as a single
  cancellation-aware coroutine instead of a four-step `.then(...).except(...)`
  pipeline. The coroutine explicitly checks the current `CancelationRef` after
  each await so stale parent-node loads cannot update the table after a newer
  selection. `PropertyContext` now carries the UI executor, and the
  reference/channel dropdown choice flow uses `MakeAsyncChoiceHandler` to spawn
  `FetchNodeNamesRecursiveAsync` while preserving the callback-shaped
  `aui::EditData::AsyncChoiceHandler` UI contract.
  `TransportPropertyDefinition::HandleEditButton` now spawns a UI-executor
  coroutine and awaits the modal transport dialog instead of continuing with a
  `.then(...)` callback. The Qt transport dialog result path now uses
  `StartMappedModalDialog(...)` to return the accepted
  `transport::TransportString` after `TransportDialog::accept()` saves the
  model, and rejects canceled dialogs without a `.then(...)` continuation.
  Regression coverage: `client/properties/property_defs_unittest.cpp` and
  `client/properties/transport/qt/transport_dialog_unittest.cpp`.
- **OPC UA outbound session adapter** migrated
  (`common/opcua/client_session.{h,cpp}`).
  `opcua::ClientSession` now exposes
  coroutine-native lifecycle methods (`ConnectAsync`, `DisconnectAsync`,
  `ReconnectAsync`) plus `Coroutine{View,Attribute,Method}Service`
  implementations. The legacy `SessionService` promises and callback-based
  SCADA service methods remain as compatibility boundaries that delegate into
  those `Awaitable` bodies via `ToPromise`/`CoSpawn`. Regression coverage:
  `common/opcua/client_session_unittest.cpp`.
- **export/configuration Excel command flows** migrated
  (`client/export/configuration/excel_configuration_commands.{h,cpp}`).
  `ExportConfigurationCommand::Execute` / `ExportTo` and
  `ImportConfigurationCommand::Execute` / `ImportFrom` remain
  promise-returning compatibility entry points, but now delegate to
  executor-pinned coroutine bodies (`ExecuteAsync`, `ExportToAsync`,
  `ImportFromAsync`). Save/open prompts, export-data collection,
  import diff building, confirmation prompts, `ApplyDiffData`, and
  `ResourceError` reporting now run as linear `co_await` flows instead of
  `.then(...).then(CatchResourceError(...))` chains. The
  `ExportConfigurationModuleContext` now carries the client executor, and
  `client_export_configuration` links `transport` for coroutine awaitable
  headers, matching the existing coroutine-enabled client modules.
  Regression coverage:
  `client/export/configuration/export_configuration_module_unittest.cpp`.
- **export/configuration export data builder** migrated
  (`client/export/configuration/export_data_builder.{h,cpp}`).
  `ExportDataBuilder::Build()` is now a thin promise compatibility wrapper over
  value-owned coroutine helpers, so inline temporary builders used by command
  code cannot outlive their coroutine frames. The recursive node hierarchy
  export now awaits each `FetchNode(...)` through the caller's executor instead
  of building nested `make_all_promise(...).then(...)` lists, while preserving
  namespace filtering and the exported data shape. Regression coverage:
  `client/export/configuration/export_data_builder_unittest.cpp`.
- **production rejected-promise compatibility audit** migrated
  (`client/properties/property_service.cpp`,
  `client/export/configuration/export_data_builder.cpp`,
  `client/tools/screenshot_generator/dialog_capture.cpp`). The remaining
  real workflow surfaces that rejected before entering coroutine code now
  reject from coroutine-backed promise bodies instead: missing executors in
  `PropertyService::GetChildPropertyDefs(...)` and `ExportDataBuilder::Build()`
  are reported by their compatibility coroutines, and screenshot capture's
  defensive `NullTaskManager` methods reject through coroutine-backed promises
  if a captured dialog unexpectedly posts a task. The Wt add-favourites dialog
  stub now uses the centralized Wt unsupported-dialog helper instead of its own
  direct rejection. Regression coverage:
  `client/properties/property_defs_unittest.cpp`,
  `client/export/configuration/export_data_builder_unittest.cpp`, and
  `client/favorites/wt/add_favourites_dialog_unittest.cpp`.
- **export/csv command flow** migrated
  (`client/export/csv/csv_export_command.{h,cpp}`). `RunCsvExport(...)`
  remains the public `promise<void>` compatibility entry point, but the save
  prompt, CSV parameter dialog, file generation, failure dialog, completion
  prompt, and optional associated-program launch now run as one
  executor-pinned coroutine body. `CsvExportContext` gained an injectable
  dialog runner so tests can cover the command flow without automating the
  platform modal dialog. `client_export_csv` now links `transport` for
  coroutine awaitable headers and its Qt/Wt unit tests link the matching AUI
  target. Regression coverage:
  `client/export/csv/csv_export_command_unittest.cpp`.
- **export/csv Qt parameter dialog result mapping** migrated
  (`client/export/csv/qt/csv_export_dialog.cpp`). `ShowCsvExportDialog(...)`
  now uses `StartMappedModalDialog(...)` to map accepted dialog state directly
  to `CsvExportParams`, persist the accepted params into the profile before the
  dialog is scheduled for deletion, and reject canceled dialogs without a
  `.then(...)` continuation. Regression coverage:
  `client/export/csv/qt/csv_export_dialog_unittest.cpp`.
- **ui/common `ExpandGroupItemIds` and main consumers** migrated
  (`client/ui/common/client_utils.{h,cpp}`,
  `client/main_window/window_definition_builder.{h,cpp}`,
  `client/main_window/main_window_util.cpp`,
  `client/components/summary/summary_model.{h,cpp}`). The recursive group
  expansion now has an executor-pinned `ExpandGroupItemIdsAsync(...)`
  coroutine, with the legacy `promise<NodeIdSet>` helper kept as a wrapper.
  Window-definition building gained coroutine overloads, default node commands
  await those overloads directly inside their existing `CoSpawn` bodies, and
  `SummaryModel` now receives the controller executor so dropped group items
  expand through the same coroutine helper. Regression coverage:
  `client/main_window/window_definition_builder_unittest.cpp` and
  `client/components/summary/summary_model_unittest.cpp`.
- **main_window command confirmation / rename flows** partially migrated
  (`client/main_window/main_menu_model.cpp`,
  `client/main_window/main_window_commands.{h,cpp}`,
  `client/main_window/main_window_module.cpp`). Page revert confirmation,
  language-restart confirmation, current-page rename, and active-window rename
  now run as executor-pinned coroutine bodies instead of `.then(...)` /
  `BindPromiseExecutor(...)` continuations. The public command-handler surface
  stays unchanged, and regression coverage for the confirm/cancel page-revert
  paths lives in `client/main_window/page_commands_unittest.cpp`.
- **remaining main_window promise wrappers** migrated
  (`client/main_window/base_main_window.{h,cpp}`,
  `client/main_window/page_commands.{h,cpp}`,
  `client/main_window/selection_commands.{h,cpp}`,
  `client/main_window/main_window_commands.{h,cpp}`,
  `client/main_window/main_window_module.cpp`). `BaseMainWindow::OpenView`
  now delegates to a coroutine body that awaits file download completion before
  opening the view, while keeping `promise<OpenedViewInterface*>` as the
  public compatibility surface. The selection-command open-group,
  open-window, and "open containing node" flows now await the coroutine
  window-definition builders and `MainWindowInterface::OpenView` directly.
  `PageCommands::RenameCurrentPage` and the `MainWindowCommands` current-page /
  active-window rename helpers now dispatch fire-and-forget coroutines from
  their command handlers instead of exposing ignored promise-returning wrappers.
  Page rename prompts gained an injectable prompt runner so the accepted and
  rejected command paths are covered without driving a real modal dialog.
  `BaseMainWindow::OpenView`, `SelectionCommands::OpenViewContainingNode`, and
  `OpenedView::GetOpenWindowDefinition` remain promise-shaped compatibility
  boundaries because callers still await their opened-view/window-definition
  results across module interfaces. Regression coverage lives in
  `client/main_window/main_window_unittest.cpp`,
  `client/main_window/main_window_module_unittest.cpp`, and
  `client/main_window/page_commands_unittest.cpp`; the OpenView tests now poll
  until promise readiness rather than draining forever through repeating
  `OpenedView` timers.
- **favorites Add URL flow** migrated
  (`client/favorites/favourites_view.cpp`,
  `client/favorites/favourites_url.{h,cpp}`). `FavouritesView::AddUrl`
  now awaits the prompt and invalid-URL error dialog in an executor-pinned
  coroutine while keeping the public `promise<>` entry point. URL validation
  and placement were split into `AddUrlToFavourites(...)` so the selected
  folder/window/default-folder rules are covered without automating a modal
  input dialog. Regression coverage:
  `client/favorites/favourites_url_unittest.cpp` for invalid URLs, default
  folder insertion, selected-folder insertion, and selected-window parent
  insertion.
- **filesystem/FileSynchronizer download branch** migrated
  (`client/filesystem/file_synchronizer.{h,cpp}`). The outdated-file read and
  write path now runs as an executor-pinned coroutine that awaits
  `node.scada_node().read(...)` instead of attaching success/error `.then(...)`
  callbacks. The public observer-driven synchronizer behavior remains
  fire-and-forget: successful downloads write bytes and update local timestamps,
  failed downloads log and leave the local cache untouched, and already-current
  files skip the server read. `FileSynchronizerContext` now carries the client
  executor required by the coroutine body. Regression coverage:
  `client/filesystem/file_synchronizer_unittest.cpp`.
- **components/watch save/export and history event source flows** migrated
  (`client/components/watch/watch_view.cpp`,
  `client/components/watch/watch_history_event_source.cpp`).
  `WatchView::SaveLog` now awaits `SelectSaveFile` in an executor-pinned
  coroutine before saving the model log, while keeping the public `promise<>`
  command path. `WatchHistoryEventSource::Start` now spawns a coroutine that
  awaits `read_event_history(...)`, checks `CancelationRef` after the await to
  preserve stale-read suppression, and delivers events on the client executor.
  Regression coverage:
  `client/components/watch/watch_history_event_source_unittest.cpp`.
- **components/write write pipeline** migrated
  (`client/components/write/write_model.{h,cpp}`). Manual, two-stage select,
  confirmation, final write, and error-dialog completion paths now use
  executor-pinned coroutine bodies instead of `BindStatusCallback`,
  `BindPromiseExecutor`, and weak-bound continuation glue. The coroutine
  helpers preserve the old lifetime behavior by carrying only `weak_ptr`
  references across service/dialog awaits, except for the error dialog path
  where the existing copied completion handler behavior is retained.
  Regression coverage: `client/components/write/write_model_unittest.cpp`.
- **components/login controller and Qt dialog completion** migrated
  (`client/components/login/login_controller.{h,cpp}`,
  `client/components/login/qt/login_dialog.cpp`). Session connect, auto-login
  info completion, force-logoff retry, and login-error reporting now run through
  executor-pinned coroutine helpers instead of promise continuations. Controller
  callbacks keep their previous lifetime rules: service/dialog awaits use
  `weak_ptr` drops, while the post-success completion handler remains copied
  across the optional auto-login message. `ExecuteLoginDialog` now awaits dialog
  completion in a coroutine before scheduling `deleteLater()`. Regression
  coverage: `client/components/login/login_controller_unittest.cpp`.
- **graph horizontal-range history read** migrated
  (`client/graph/metrix_data_source.{h,cpp}`). `MetrixDataSource` now owns an
  `AnyExecutor` (defaulting to a thread executor for existing graph
  construction) and schedules earliest-timestamp history reads as a coroutine
  instead of attaching a `Cancelation::Bind(...).then(...)` continuation. The
  coroutine checks the captured `CancelationRef` before and after awaiting the
  history read, preserving stale-read suppression when the data source changes
  or is destroyed. `client_graph` now links `transport` for coroutine
  awaitable headers. Regression coverage:
  `client/graph/graph_view_unittest.cpp`.
- **clipboard recursive paste/copy helpers** migrated
  (`client/clipboard/clipboard_util.cpp`). `CopyNodesToClipboard` now spawns a
  coroutine that awaits each node fetch before writing the clipboard formats,
  while preserving the existing fire-and-forget public command surface.
  `PasteNodesFromNodeStateRecursive(...)` and `PasteNodesFromNodeTree(...)`
  remain `promise<>` compatibility boundaries but now delegate to recursive
  coroutine bodies that await `TaskManager::PostInsertTask`, strip inverse
  references before insertion, and assign inserted parent IDs to children
  before recursing. `client_clipboard` now links `transport` for coroutine
  awaitable headers. Regression coverage:
  `client/clipboard/clipboard_util_unittest.cpp`.
- **clipboard/main-window paste validation** migrated
  (`client/clipboard/clipboard_util.cpp`,
  `client/main_window/opened_view_commands.{h,cpp}`). Clipboard payload
  validation and opened-view paste guards no longer return immediate
  `MakeRejectedPromise()` results. `PasteNodesFromClipboard(...)` now wraps the
  clipboard read/parse path in a coroutine that throws for empty or malformed
  node-tree payloads, and `OpenedViewCommands::PasteFromClipboard()` delegates
  its privilege, selection-model, and paste-parent checks to an executor-pinned
  coroutine before awaiting the clipboard paste promise. Regression coverage:
  `client/clipboard/clipboard_util_unittest.cpp`.
- **configuration tree drop task actions** migrated
  (`client/configuration/tree/configuration_tree_drop_handler.{h,cpp}`,
  `client/configuration/{objects,devices,nodes}`). Data-item creation,
  channel assignment, and organize-reference moves now dispatch
  executor-pinned coroutine bodies from the shared drop handler and await the
  `TaskManager` promises instead of dropping them directly from the drag/drop
  callback. The nodes, hardware, and object configuration views thread the
  controller executor into the handler, while the UI-facing `DropAction`
  contract remains synchronous. Regression coverage:
  `client/configuration/tree/configuration_tree_drop_handler_unittest.cpp`.
- **configuration tree lazy child fetch** migrated
  (`client/configuration/tree/configuration_tree_model.{h,cpp}`,
  `client/configuration/tree/configuration_tree_node.{h,cpp}`,
  `client/configuration/{nodes,objects,devices}`, and
  `client/filesystem`). `ConfigurationTreeNode::FetchMore` now starts an
  executor-pinned coroutine that awaits the callback-shaped
  `NodeRef::Fetch(NodeAndChildren, callback)` through `CallbackToAwaitable`.
  The model owns the executor so fetched children are materialized on the UI
  executor, and the previous lifetime-token / stale-tree-node suppression is
  preserved before touching the model after an await. Regression coverage:
  `client/configuration/tree/configuration_tree_model_unittest.cpp` now covers
  successful materialization, loading text while pending, model destruction,
  and removal of a tree node before the delayed fetch callback resumes.
- **configuration object visible-node fetch** migrated
  (`client/configuration/objects/object_tree_model.{h,cpp}` and
  `client/configuration/objects/visible_node_model.{h,cpp}`).
  `ObjectTreeModel::CreateVisibleNode` now keeps the proxy-visible-node public
  behavior but starts an executor-pinned coroutine for delayed
  `NodeRef::Fetch(NodeOnly, callback)` completion through `CallbackToAwaitable`.
  The coroutine verifies model lifetime, current tree-node identity, and current
  visible-row ownership before installing the fetched visible node, so stale
  completions after hiding or removing a row are ignored. Empty proxy text is
  now explicitly represented as an empty string. Regression coverage:
  `client/configuration/objects/object_tree_model_unittest.cpp`.
- **method command status reporting** migrated
  (`client/main_window/configuration_commands.cpp` and
  `client/components/change_password/change_password.cpp`). Device method
  commands and user password changes now start executor-pinned coroutine bodies
  that await the SCADA method-call promises and map success/failure back through
  `ReportRequestResult(...)`, replacing the last direct
  `scada::BindStatusCallback` production call sites. `ChangePasswordContext`
  now carries the caller executor so the dialog command reports completion on
  the UI executor. Regression coverage:
  `client/main_window/configuration_commands_unittest.cpp` and
  `client/components/change_password/change_password_unittest.cpp`.
- **client UI helper compatibility wrappers** audited and tightened
  (`client/ui/common/client_utils.{h,cpp}` and
  `client/favorites`). `ExpandGroupItemIds(...)` remains a public
  `promise<NodeIdSet>` compatibility wrapper over `ExpandGroupItemIdsAsync`,
  but now enforces `max_count` during recursive traversal and returns
  immediately for a zero limit without fetching. Favourites URL insertion now
  uses the shared coroutine helper
  `AddUrlToFavouritesWithPrompt{Async}(...)`; `FavouritesView` supplies a
  lifetime token so prompt completions after view destruction do not read stale
  selection or mutate favourites. The Wt add-favourites dialog remains an
  intentional rejected-promise platform stub. Regression coverage:
  `client/ui/common/client_utils_unittest.cpp` and
  `client/favorites/favourites_add_url_unittest.cpp`.
- **node property initial fetch/update** migrated
  (`client/components/node_properties/node_property_model.cpp`). The model
  constructor now starts an executor-pinned coroutine using
  `PropertyContext::executor_`, awaits `FetchNode(node_)`, and updates the
  property tree only if its captured cancellation ref is still live. Destruction
  and node-delete handling now cancel the pending startup fetch path before
  unsubscribing, preserving the public `model_changed_handler` and
  `node_deleted` behavior without a `.then(...)` continuation. Regression
  coverage:
  `client/components/node_properties/node_property_model_unittest.cpp`.
- **Qt time range dialog result completion** migrated
  (`client/components/time_range/qt/time_range_dialog.cpp`).
  `ShowTimeRangeDialog(...)` now returns a coroutine-backed
  `promise<TimeRange>` that runs on a `MessageLoopQt` executor, waits for the
  modal dialog result, captures the selected range before scheduling
  `deleteLater()`, and propagates rejection for canceled dialogs without a
  `.then(...)` continuation. Regression coverage:
  `client/components/time_range/qt/time_range_dialog_unittest.cpp`.
- **Qt modal dialog result mapping helpers** migrated
  (`client/aui/qt/dialog_util.h`, `client/aui/qt/prompt_dialog.cpp`,
  `client/aui/qt/dialog_service_impl_qt.cpp`). A shared coroutine-backed
  `StartMappedModalDialog(...)` helper now waits for accepted/rejected dialog
  completion on a `MessageLoopQt` executor, maps accepted dialogs to the public
  return value before scheduling `deleteLater()`, rejects canceled dialogs, and
  rejects mapper exceptions. Prompt/open-file/save-file flows now use that
  helper instead of `StartModalDialog(...).then(...)`; the time range dialog was
  folded back onto the shared helper as well. `aui_qt` now links `transport`
  and `scada_core` for the awaitable helper surface. Regression coverage:
  `client/aui/qt/dialog_util_unittest.cpp`.
- **resource error dialog handling** migrated
  (`client/aui/resource_error.h`). `ShowResourceError(...)` now invokes
  `DialogService::RunMessageBox(...)` synchronously as before, then awaits the
  message-box promise in a coroutine and rethrows the original
  `ResourceError`. `HandleResourceError(...)` awaits a source promise,
  preserves success passthrough, routes failures through `ShowResourceError`,
  and no longer uses `.then(...)` / `.except(...)`. `aui` now links
  `transport` and `scada_core` because the public header exposes awaitable
  adapters. Regression coverage: `client/aui/resource_error_unittest.cpp`.
- **Wt dialog compatibility boundaries** centralized and covered
  (`client/aui/wt`, `client/components/time_range/wt`,
  `client/export/csv/wt`, `client/properties/transport/wt`). The remaining Wt
  modal-dialog surfaces are intentional unsupported platform stubs, now routed
  through `aui::wt::MakeUnsupportedDialogPromise<T>()` instead of scattered
  ad hoc rejected promises. Message boxes still resolve with `Ok`, while file
  selection, prompt, time-range, CSV-params, and transport dialogs reject
  immediately at the platform boundary. Regression coverage:
  `client/aui/wt/dialog_stub_unittest.cpp`,
  `client/components/time_range/wt/time_range_dialog_unittest.cpp`,
  `client/export/csv/wt/csv_export_dialog_unittest.cpp`, and
  `client/properties/transport/wt/transport_dialog_unittest.cpp`.

### Priority Order

1. `services/`
2. `events/`
3. `filesystem/` — `FileManagerImpl` migrated (see Status)
4. `profile/`
5. `main_window/`
6. `configuration/*`
7. `graph/`, `modus/`, and vidicon-specific modules

### Clear Next Step

Continue the production async-surface audit outside test-only helpers: the only
remaining direct rejected-promise production site is the documented centralized
Wt unsupported-dialog helper (`client/aui/wt/dialog_stub.h`). Leave that helper
as the platform boundary unless Wt gains real modal-dialog support, then
continue with the next non-test promise-chain cluster found by
`rg "\.then\(|\.except\(" client`.

### Work

- Replace direct callback/promise core service usage with the coroutine
  versions of those services.
- Normalize module wiring so service-heavy client code receives
  `CoroutineAttributeService`, `CoroutineMethodService`,
  `CoroutineHistoryService`, `CoroutineViewService`,
  `CoroutineNodeManagementService`, and coroutine-adapted session access where
  appropriate.
- Keep callback/promise core services only at compatibility boundaries and
  convert them once at module setup time with the shared adapters.
- Convert module-level async helpers to coroutine functions.
- Keep model-update points explicit so UI mutation still happens on the correct
  executor/thread.

## Phase 5: Public API Evaluation

After the internal migration is stable, decide whether the client should keep
`promise<T>` as its public async boundary or expose coroutine-native entry
points.

### Options

- Keep `promise<T>` at public boundaries for compatibility and testing.
- Add dual APIs where internals are coroutine-first but public entry points can
  still return `promise<T>`.
- Convert selected internal-only interfaces fully to awaitables.

### Recommendation

Do not make this decision during the first implementation pass. Treat it as a
follow-up design review after startup/task/service migrations land cleanly.

## Execution Model Rules

- UI mutations must happen on the same executor/thread they happen on today.
- Coroutine suspension points must not hide thread hops.
- Background work should be explicit and rare.
- Each module should own the lifetime of its in-flight coroutine work.
- Destruction paths must cancel work before releasing UI/model state.

## Testing Strategy

### Unit Tests

- Adapter tests:
  - `promise<T>` success/failure propagation
  - callback-to-awaitable success/failure/cancellation
  - executor-affinity preservation
- Startup tests:
  - successful login/start
  - canceled login
  - startup failure propagation
  - clean shutdown during in-flight startup
- Task/progress tests:
  - progress completion
  - cancellation
  - exception propagation

### Integration / UI-Adjacent Tests

- Existing `client_qt_unittests` and `client_wt_unittests`
- module-specific unit tests in `events`, `filesystem`, `profile`,
  `main_window`, `configuration`, and `graph`
- screenshot generator smoke flows that rely on local services and async model
  population

### Regression Focus

- UI thread-affinity bugs
- startup ordering regressions
- missing cancellation on shutdown
- reconnect/session lifecycle deadlocks caused by duplicate waits on the same
  completion signal
- double-completion / dangling-callback behavior during teardown

## Rollout Guidance

- Migrate one vertical slice at a time.
- Avoid mixing service-interface redesign with flow-control migration.
- Prefer migrating classes to coroutine-native internals over introducing new
  permanent adapter classes.
- Use shared adapters at legacy boundaries instead of ad hoc wrapper lambdas.
- Once a client slice moves to coroutines, switch that slice to the coroutine
  versions of core SCADA services instead of continuing to call the legacy
  callback/promise service APIs from coroutine bodies.
- If a legacy public interface must remain in place, keep the adapter thin and
  temporary while the implementation behind it becomes coroutine-first.
- Keep old and new implementations behaviorally identical before broadening
  scope.
- Land tests with each slice before moving to the next module group.

## Recommended First Implementation Slice

1. Shared adapter layer validation in client code.
2. `client_application` startup/login/shutdown coroutine internals.
3. `TaskManager` coroutine submission helpers.
4. One representative service-heavy module such as `events` or `filesystem`.

This sequence gives the best coverage of real client async behavior while
keeping the migration incremental and testable.
