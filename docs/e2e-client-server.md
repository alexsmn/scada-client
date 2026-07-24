# Qt Client / Server E2E Test Design

This document describes the intended always-on end-to-end test coverage for the
real Qt desktop client (`client.exe`) connecting to the real SCADA server
(`server.exe`) over the production remote-session API.

The goal is to exercise the actual process boundary and login/session bootstrap
path without replacing either binary with in-process mocks.

## Run Instructions

The suite builds and runs on **Windows and macOS** (it is skipped on Linux —
see `client/test/e2e/CMakeLists.txt`, guard `if(NOT WIN32 AND NOT APPLE)`).

### Windows

Run the client/server E2E tests from a Windows build environment. The examples
below assume the repository is checked out at `C:\tc\scada` and the
`release-dev` preset builds into `C:\tc\scada\build\ninja-dev`.

Build the test executable and required child processes:

```cmd
cd /d C:\tc\scada
cmake --build --preset release-dev --target client_server_e2e_tests
```

Point the server at an externally issued signed license before running the
tests. For local, non-GCP-bound licenses, disable the GCP binding check and let
the server continue to validate the license signature and validity window:

```cmd
set SCADA_SERVER_LICENSE_FILE=C:\path\to\license.json
set SCADA_SERVER_LICENSE_REQUIRE_GCP_BINDING=false
```

Run the full client/server E2E suite:

```cmd
cd /d C:\tc\scada\build\ninja-dev\bin\RelWithDebInfo
client_server_e2e_tests.exe --gtest_brief=1
```

Run a focused test while iterating:

```cmd
client_server_e2e_tests.exe --gtest_filter=*OperatorUseCases* --gtest_brief=1
client_server_e2e_tests.exe --gtest_filter=*Connect_Success* --gtest_brief=1
client_server_e2e_tests.exe --gtest_filter=*Connect_BadPassword* --gtest_brief=1
```

CTest can also run the registered test target from the build tree:

```cmd
cd /d C:\tc\scada\build\ninja-dev
ctest -C RelWithDebInfo -R client_server_e2e_tests --output-on-failure
```

### macOS

The target is created only when the configure builds **both** the client and the
server, i.e. with `BUILD_CLIENT=ON`. Use the client-enabled configure preset
(`macos-local-client`); the plain `macos-local` preset sets `BUILD_CLIENT=OFF`,
so `client_qt` and the E2E target are not created. `sqlite3` must be on `PATH`
(the CMake step finds it via `find_program`).

```sh
cmake --preset macos-local-client
cmake --build --preset client-tests-macos-local --target client_server_e2e_tests
```

Set the same license environment variables as on Windows (the dev `license.json`
written next to the built `server` by `init_runtime_data.cmake` works locally
with GCP binding disabled), then run the binary from the build output directory:

```sh
export SCADA_SERVER_LICENSE_FILE="$PWD/build/macos-local-client/bin/RelWithDebInfo/license.json"
export SCADA_SERVER_LICENSE_REQUIRE_GCP_BINDING=false
build/macos-local-client/bin/RelWithDebInfo/client_server_e2e_tests --gtest_brief=1
# focused: ... --gtest_filter=*Connect_Success* --gtest_brief=1
```

On failure, the harness prints the preserved temporary workspace path. Inspect
that directory for `ServerLogs/`, `ClientLogs/`, status marker files, and the
operator use-case report when that test was running.

## Viewing a run's telemetry

By default the suite exports nothing: assertions read the plain-text logs under
the preserved workspace, and no process is aimed at a collector. Setting
`SCADA_E2E_OTLP_ENDPOINT` opts a run into OpenTelemetry export so it can be
inspected in a viewer instead of grepped:

```sh
SCADA_E2E_OTLP_ENDPOINT=localhost:4317 \
  build/macos-local-client/bin/RelWithDebInfo/client_server_e2e_tests \
  --gtest_filter='*Connect_Success_LoadsObjectTree/OpcUa_Cluster*'
```

The harness then patches every server process's `server.json` with a `metrics`
block (`traces.enabled`, `sampling_ratio` 1.0) and a `log.otlp` block, and
passes `--otlp-endpoint` to the client. Each process gets its own
`service_name`, so a Cluster run appears as one service per tier:

| Service | Process |
|---|---|
| `scada-e2e-server` | the SingleTier server |
| `scada-e2e-proxy` | the client-facing aggregating proxy |
| `scada-e2e-config` | the config tier |
| `scada-e2e-historian` | the historian |
| `scada-e2e-iec60870` / `-modbus` / `-iec61850` | the device edges |
| `scada-e2e-filesystem` | the file store |
| `scada-client` | the Qt client |

Any OTLP/gRPC receiver on that endpoint works — including the Grafana Cloud
bridge in [`dev/telemetry/`](../../dev/telemetry/README.md), which converts to
the OTLP/HTTP the hosted gateway requires. All-signal viewers
([otel-desktop-viewer](https://github.com/CtrlSpice/otel-desktop-viewer), UI on
`:8000`; [otel-tui](https://github.com/ymtdzzz/otel-tui), terminal) show logs,
traces and metrics together; Jaeger shows traces only. Note that
otel-desktop-viewer stores telemetry in memory unless started with `--db`, and
an E2E run is over in seconds — pass `--db` if you want the run to still be
there when you go looking.

### Coverage and gaps

A Cluster run produces a genuine cross-tier waterfall: a single trace carries
the proxy's outbound `opcua.client/Browse` and the edge's inbound
`opcua.server/Browse`, linked by the traceparent the OPC UA `additionalHeader`
carries (see
[`scada-server-framework/docs/tracing.md`](../../scada-server-framework/docs/tracing.md)).

What is **not** covered:

- **The client emits metrics only.** It runs no trace sink and no OTLP log
  sink, so every waterfall starts at the server and the client's own log
  records never reach the viewer. Closing this is scoped separately in
  [`client-telemetry-gaps.md`](client-telemetry-gaps.md).
- The client's default 60 s export period outlives a test case, so the harness
  passes `--otlp-export-interval-ms=2000`. Without it `scada-client` is absent
  from the viewer entirely.
- The known server-side gaps in `tracing.md` still apply — notably that
  `HistoryService` carries no `ServiceContext`, so a proxy→historian
  HistoryRead starts a new trace root rather than continuing the client's.

## Goals

- Launch the real `server.exe` in a temporary test workspace.
- Launch the real Qt desktop client `client.exe`.
- Prove that the client can establish a real SCADA session against the server.
- Run under normal Windows CTest/CI without manual interaction.
- Keep the test deterministic and isolated from developer-local registry state.

## Non-goals

- Full GUI workflow automation after login.
- Coverage for the Wt client in the same test target.
- Screenshot/image comparison.
- Full protocol matrix coverage beyond the SCADA remote-session and OPC UA
  paths.

## Scope

The first version covers the Qt desktop application only.

`client.exe` already uses the shared `ClientApplication` bootstrap path:

1. `app/qt/main.cpp` creates `QApplication`, translation/style helpers, and
   the `MessageLoopQt` executor.
2. `ClientApplication::Start()` invokes the login handler.
3. `ExecuteLoginDialog(...)` creates `LoginController`, which creates
   `DataServices` and calls `SessionService::Connect(...)`.
4. On success, `ClientApplication::PostLogin()` builds the node/event/timed-data
   stack and opens the first profile page.

The E2E test is concerned with steps 2-4.

## High-Level Behavior

The always-on E2E test is a Windows-only GoogleTest executable that launches
both child processes and owns their lifecycle.

For each test:

1. Create a unique temp workspace.
2. Materialize a server fixture under that workspace.
3. Start `server.exe --param=<workspace>/server.json`.
4. Wait until the configured TCP session port accepts connections.
5. Write a client test-settings file with the selected backend, host, user,
   password, and auto-login enabled.
6. Start `client.exe` with test-only startup flags that point at:
   - the test-settings file,
   - a ready-file path,
   - a login-status-file path,
   - a per-test client log / dump directory.
7. Wait for one of:
   - ready file: login/startup succeeded,
   - status file with failure text: login failed deterministically,
   - process exit/crash,
   - timeout.
8. Assert the expected outcome, including a short post-login or post-rejection
   stability window so delayed server exits are caught, then terminate both
   processes cleanly.

Per-test diagnostics are written into the temp workspace instead of user-global
locations:

- `ServerLogs/` contains `server.exe` logs and server crash dumps.
- `ClientLogs/` contains `client.exe` logs and client crash dumps.
- marker files such as ready/status outputs remain at workspace root.

## Test-Only Client Hook

The production Qt client remains the launched binary, but it needs a narrow
test-only seam so CI does not depend on interactive GUI input or shared
registry state.

### Inputs

The Qt startup path accepts these test-only flags:

- `--test-settings-file=<path>`
- `--test-ready-file=<path>`
- `--test-status-file=<path>`
- `--test-log-dir=<path>`
- `--test-operator-use-cases-file=<path>`
- `--test-historical-timed-data-file=<path>`
- `--test-historical-timed-data-end=<time>`

These are only used by the E2E harness.

When `--test-historical-timed-data-file` is present, the client opens the real
timed-data view on the historized, simulated analog item TIT.4, waits for the
historical HistoryRead to populate rows, and then
exports them to the given path using the view's own Export-to-CSV writer (the
same `ExportToCsv` the `ID_EXPORT_CSV` command runs, minus the interactive
save-file dialog). The report is a CSV: a header row plus one row per historical
sample. `--test-historical-timed-data-end` (a `scada::base::Time` internal
value the harness records *before launching the client*) pins the view to a
fixed past window ending there instead of the default Day window: live
monitored-item updates all carry timestamps after the client launched, so they
fall outside the window and only server-stored history (the historian, in the
Cluster topology) can produce rows — without it the check could pass off
client-side live buffering even when proxy history routing was broken.

When `--test-log-dir` is present, the client overrides its normal
`%LOCALAPPDATA%\Telecontrol\SCADA Client\logs` path and writes both component
logs and crash dumps into the supplied per-test directory.

When `--test-operator-use-cases-file` is present, the client runs an
E2E-only operator use-case smoke pass after login/bootstrap and writes a
line-oriented report. The pass implements the testing instructions from
`docs/use-cases.md` for UC-1 through UC-19. UC-1 through UC-11 open or verify
operator surfaces against the live server session. UC-12 through UC-19 verify
deterministic configuration, administration, authentication, layout, and
debugging command/window registration. It is not pixel comparison or UI
clicking; it proves that the launched production client can construct the
registered controllers and expose the expected commands after bootstrap.

### Settings behavior

When `--test-settings-file` is present, the login dialog must not use
`RegistrySettingsStore`. Instead it uses a file-backed `SettingsStore`
implementation populated from the provided file.

The settings file must provide at least:

- `ServerType=Scada` or `ServerType=OpcUa`
- `Host:Scada=localhost:<port>`
- `Host:OpcUa=127.0.0.1:<port>`
- `User=root`
- `Password=...`
- `AutoLogin=true`

It may optionally provide:

- `SecurityMode=None|Auto|SignAndEncrypt` — OPC UA endpoint security selection.
  When `Auto` or `SignAndEncrypt`, the OPC UA backend runs `GetEndpoints`
  discovery and selects an endpoint before connecting. Ignored by the Scada
  (gRPC) backend.

This keeps test runs isolated from `HKEY_CURRENT_USER\Software\Telecontrol\Workplace`.

### Auto-login failure behavior

Normal interactive behavior is unchanged.

In E2E test mode, auto-login failures must not leave the login dialog waiting
for a human. The login path should:

- report the failure into `--test-status-file`,
- resolve startup as a failed login / canceled login,
- allow the app to exit cleanly.

This is required for a deterministic `BadPassword` test.

### Status signals

Two files are used so the harness can distinguish success from failure without
window automation:

- `--test-ready-file`
  Written only after `ClientApplication::Start()` completes successfully.
- `--test-status-file`
  Written on explicit login outcomes such as:
  - `success`
  - `failure: Bad_WrongLoginCredentials`
  - `canceled`

The ready file is the positive proof that the real client completed login and
post-login bootstrap against the server.

The harness still treats child-process exit as a first-class signal: if the
client or server dies before the expected status is observed, the test fails and
reports the child exit code when available.

## Server Fixture

The E2E harness uses a temp copy of the checked-in server data fixture rather
than the in-repo runtime directories.

Required fixture contents:

- configuration database,
- filesystem directory,
- optional certificates if the copied baseline expects them,
- server parameter file with the SCADA session listener enabled.

The harness rewrites the remote-session and OPC UA ports in the temp
`server.json` so each run can use free local TCP ports.

The harness also rewrites the server log directory into the temp workspace so
server logs and server crash dumps stay with the rest of the test artifacts.

The first version uses the built-in `root` user path already exercised by the
existing server-side tests.

The current harness disables optional subsystems such as Vidicon in the temp
`server.json` while enabling the SCADA remote-session and OPC UA endpoints.

## Server topology (single tier vs cluster)

Every test is parametrized over two axes — the client backend protocol and the
**server topology** — so each `TEST_P` runs as
`<Protocol>_<Topology>` (e.g. `Remote_SingleTier`, `OpcUa_Cluster`):

- **SingleTier** — one device tier process (`scada-iec104`) on the client-facing
  ports, playing the whole server with its own local config DB. Covers the
  framework login/browse/profile flows plus one live protocol.
- **Cluster** — the real ADR-0001 tier split, standing up six processes: a
  `scada-config` tier owning the configuration namespace; the three device edges
  `scada-iec104` / `scada-modbus` / `scada-iec61850` (each a config client
  running one driver, fetching config from the config tier as the multi-session
  `svc` user; edges run no history module); a `scada-historian` that
  pull-collects from an edge (`historyCollection.sources`) and self-registers
  with the proxy via RegisterServer2 advertising the `HD` capability, so the
  proxy's history-link module (`historyLink`, svc) routes client HistoryRead
  to it (the proxy's aggregation skips HD registrants); and a
  client-facing aggregating `scada-proxy` that aggregates the three edges
  anonymously. The iec104 and iec61850 edges use static `aggregation.servers`
  entries; the modbus edge is aggregated **dynamically** — it self-registers
  with the proxy via OPC UA RegisterServer (`opcua.register_with_url` +
  `advertise_url` + a unique `application_uri`) and the proxy's
  DiscoveryRegistry reconcile loop stands the downstream up. That keeps
  permanent E2E coverage of the discovery path the Windows on-prem deployment
  (`scada-setup`) wires every edge with; cluster startup waits for the proxy
  log line `Aggregating registered downstream` before tests run. The client
  connects only to the proxy, which re-exposes the edges' address space
  through OPC UA aggregation. The tier configs mirror
  `gcp/free-tier/multitier/configs/*.json`.

Each tier is launched with the shared `ServerTier` harness in
`common/test/e2e/e2e_server_process.h` — a distinct tier binary plus an arbitrary
`server.json` `configure` lambda — bound to the client target's paths and license
via `MakeTierContext()`. The proxy reuses the harness's built-in server slot
(`server_` / `workspace_` / the client-facing ports), so every existing assertion
— auth logs, post-connect stability, the client-facing endpoint — targets the
process the client actually connects to, unchanged. `StartCluster()` in
`client_server_e2e_test_support.cpp` wires the whole topology.

The point of the Cluster axis is to prove the client behaves identically whether
the server is one process or a multi-process cluster behind a northbound proxy.

**Current cluster coverage.** Connect/login, operator use-cases, bad-password,
and object-tree loading pass through the real cluster; the client also sees the
full aggregated device tree. The deeper content assertions are gated pending
server-tier gaps in the config-client remote-config / aggregation path (each test
carries a comment and skips accordingly):

- **hardware-tree devices** — skipped: the aggregated devices don't yet surface
  as online through the cluster.
- **historical timed-data** — runs only under Cluster (a single device tier
  owns no history) and asserts *historian-served* rows: the harness freezes
  the view's window end before launching the client
  (`--test-historical-timed-data-end`), so live buffering can't populate it —
  the rows must round-trip edge → historian collection → proxy history link →
  client.
- **nested object-tree labels** — skipped under Cluster: the nested
  station/group DisplayNames don't fully come through the aggregation
  attribute path yet. Labels still run under SingleTier.
- **profile save** — skipped under Cluster: the client→proxy→edge→config
  write-through isn't wired yet. Still runs under SingleTier.

These are tracked server-framework follow-ups, not client issues; the
`common/test/e2e` remote-config Browse batching (`Bad_TooManyOperations`) was one
such gap and is already fixed. The OPC UA discovery/security tests remain
protocol-gated (skip on the Remote/gRPC backend under either topology).

## Assertions

### `Connect_Success`

Expected behavior:

- `server.exe` starts and listens on the configured session port.
- `client.exe` reads the test settings and auto-submits the login.
- the remote session is established successfully,
- the client writes `success` to the status file,
- the client writes the ready file after `ClientApplication::Start()` succeeds,
- the client remains alive until the harness shuts it down,
- the server remains alive for a short post-connect stability window after the
  client finishes login, so delayed crashes are treated as test failures.

Current harness details:

- waits up to 30 seconds for the server listener to accept TCP connections,
- waits up to 30 seconds for the client status file / login outcome,
- waits up to 10 seconds for a server auth signal in the log directory
  (`Authorization succeeded` or `CreateSession completed`),
- then holds both `client.exe` and `server.exe` alive for a 10 second
  post-connect stability window.

### `Connect_BadPassword`

Expected behavior:

- `server.exe` starts and listens on the configured session port.
- `client.exe` attempts the auto-login with the supplied bad password.
- the client writes a failure status such as
  `failure: Bad_WrongLoginCredentials`,
- the client does not write the ready file,
- the server remains alive after rejecting the bad credentials,
- the client exits cleanly or becomes terminable immediately after the failed
  login path resolves.

Current harness details:

- still requires the server listener to start normally before the client runs,
- verifies the server does not log successful authorization,
- holds the server alive for a 10 second post-rejection stability window so
  failed-auth connect crashes are also caught.

### `Connect_Success_WithDiscoveryAutoSecurity` (OPC UA only)

Skipped for the Scada backend. With `SecurityMode=Auto`, exercises the full
discovery-driven connect against the real server:

- the client runs `GetEndpoints` discovery, selects the server's advertised
  (SecurityPolicy=None) endpoint, and completes login,
- the same `Connect_Success` success signals hold (startup-completed log,
  empty/`success` status, client still running, server auth log),
- the client log records the post-activation `NamespaceArray` read, confirming
  the discovery + namespace path ran,
- both processes stay alive for the post-connect stability window.

### `Connect_SignAndEncryptRejectedWhenServerOffersNone` (OPC UA only)

Skipped for the Scada backend. With `SecurityMode=SignAndEncrypt` against a
server that advertises only a None endpoint, endpoint selection must fail:

- the client reports a `failure: ...` status (no compatible secure endpoint),
- the server never logs `OPC UA session activated`,
- the server remains alive for the post-rejection stability window.

### `OperatorUseCases_OpenRegisteredSurfaces`

Expected behavior:

- `server.exe` and `client.exe` complete the same real login/bootstrap path as
  `Connect_Success`,
- the client writes an operator use-case report with `operator-use-cases: ok`,
- every use case from the diagram (`UC-1` through `UC-19`) has an
  `ok` line in the report,
- the client and server remain alive for the post-connect stability window.

Current harness details:

- opens constructible operator views inside the running Qt client rather than
  clicking through the GUI,
- verifies command-only and platform-specific operator capabilities through the
  same registered command/window metadata the real UI uses,
- starts the client in debug mode so UC-15 can verify the debug-inspection
  command registration,
- runs for both SCADA remote-session and OPC UA back-end parameters.

### `Connect_Success_DisplaysHistoricalTimedData`

Expected behavior:

- the harness historizes + simulates TIT.4 before launch (via
  `EnableSimulatedHistory`), so the historian pull-collects a steady stream of
  samples into its analog historical DB while the tiers start up,
- `client.exe` completes the same real login/bootstrap path as
  `Connect_Success`,
- the client opens the real timed-data view over a window frozen to end before
  the client launched (`--test-historical-timed-data-end`), reads the
  historian's samples back through the active backend, and exports them via
  the view's Export-to-CSV writer,
- the exported CSV contains at least one historical data row (beyond the
  header) — necessarily historian-served, since live buffering falls outside
  the frozen window,
- the client and server remain alive for the post-connect stability window.

Runs only under the Cluster topology (a single device tier owns no history).
The data items live on the edges and the stored history on the historian, so a
passing run proves the proxy's discovery-driven history link (RegisterServer2
`HD` capability → history-link module → `RemoteHistoryService`) end to end.

## Process and Timeout Policy

The harness owns child-process creation, wait loops, and cleanup.

Required behavior:

- capture child stdout/stderr or logs into the temp workspace for diagnosis,
- use bounded startup and shutdown timeouts,
- treat unexpected client/server exit as test failure,
- include process-exit diagnostics in assertion failures when the exit code is
  available,
- keep the successful session alive long enough to catch short delayed server
  crashes after connect,
- keep the server alive long enough after rejected credentials to catch delayed
  auth-path crashes as well,
- forcibly terminate lingering child processes during teardown,
- always clean temp files after the test unless preservation is requested for
  debugging.

## CI Integration

The E2E executable is registered with CTest and runs in the normal Windows
`test-release-dev` path. It also builds and runs on macOS (see Run Instructions).

Constraints:

- Windows and macOS only (skipped on Linux),
- depends on the built client and server binaries (`client.exe` / `server.exe`
  on Windows; `client` / `server` on macOS),
- should run serially or with unique temp workspaces to avoid fixture and port
  collisions,
- must not mutate repo-tracked files.

## Why a Test Hook Instead of UI Clicking

Pure black-box GUI automation was intentionally rejected for the first
always-on version because it would be significantly more fragile:

- login fields are populated from settings and shown in a modal dialog,
- failures surface through message boxes,
- registry-backed MRU/autologin state is shared across local runs,
- timing differs across CI machines.

The file-backed settings hook keeps the launched binary real while reducing the
test to deterministic process-level orchestration.
