# Qt Client / Server E2E Test Design

This document describes the intended always-on end-to-end test coverage for the
real Qt desktop client (`client.exe`) connecting to the real SCADA server
(`server.exe`) over the production remote-session API.

The goal is to exercise the actual process boundary and login/session bootstrap
path without replacing either binary with in-process mocks.

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
- Broad protocol coverage beyond the SCADA remote-session path.

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
5. Write a client test-settings file with SCADA backend, host, user, password,
   and auto-login enabled.
6. Start `client.exe` with test-only startup flags that point at:
   - the test-settings file,
   - a ready-file path,
   - a login-status-file path.
7. Wait for one of:
   - ready file: login/startup succeeded,
   - status file with failure text: login failed deterministically,
   - process exit/crash,
   - timeout.
8. Assert the expected outcome, then terminate both processes cleanly.

## Test-Only Client Hook

The production Qt client remains the launched binary, but it needs a narrow
test-only seam so CI does not depend on interactive GUI input or shared
registry state.

### Inputs

The Qt startup path accepts these test-only flags:

- `--test-settings-file=<path>`
- `--test-ready-file=<path>`
- `--test-status-file=<path>`

These are only used by the E2E harness.

### Settings behavior

When `--test-settings-file` is present, the login dialog must not use
`RegistrySettingsStore`. Instead it uses a file-backed `SettingsStore`
implementation populated from the provided file.

The settings file must provide at least:

- `ServerType=Scada`
- `Host:Scada=localhost:<port>`
- `User=root`
- `Password=...`
- `AutoLogin=true`

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

## Server Fixture

The E2E harness uses a temp copy of the checked-in server data fixture rather
than the in-repo runtime directories.

Required fixture contents:

- configuration database,
- filesystem directory,
- optional certificates if the copied baseline expects them,
- server parameter file with the SCADA session listener enabled.

The harness rewrites the session port in the temp `server.json` so each run can
use a free local TCP port.

The first version uses the built-in `root` user path already exercised by the
existing server-side tests.

## Assertions

### `Connect_Success`

Expected behavior:

- `server.exe` starts and listens on the configured session port.
- `client.exe` reads the test settings and auto-submits the login.
- the remote session is established successfully,
- the client writes `success` to the status file,
- the client writes the ready file after `ClientApplication::Start()` succeeds,
- the client remains alive until the harness shuts it down.

### `Connect_BadPassword`

Expected behavior:

- `server.exe` starts and listens on the configured session port.
- `client.exe` attempts the auto-login with the supplied bad password.
- the client writes a failure status such as
  `failure: Bad_WrongLoginCredentials`,
- the client does not write the ready file,
- the client exits cleanly or becomes terminable immediately after the failed
  login path resolves.

## Process and Timeout Policy

The harness owns child-process creation, wait loops, and cleanup.

Required behavior:

- capture child stdout/stderr or logs into the temp workspace for diagnosis,
- use bounded startup and shutdown timeouts,
- treat unexpected client/server exit as test failure,
- forcibly terminate lingering child processes during teardown,
- always clean temp files after the test unless preservation is requested for
  debugging.

## CI Integration

The E2E executable is registered with CTest and runs in the normal Windows
`test-release-dev` path.

Constraints:

- Windows only,
- depends on built `client.exe` and `server.exe`,
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
