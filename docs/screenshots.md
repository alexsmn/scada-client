# Screenshot generator

Low-level design of the offline screenshot generator plus the
day-to-day workflow for managing images under `scada-docs/img/`.

For the high-level "what is FR-21", see `design.md` §5.1 "Regenerating
the doc screenshots". This doc is the authoritative reference when you
are modifying the generator itself or adding new captures.

## What the generator is

`client/tools/screenshot_generator/screenshot_generator.exe` is a Qt
test binary that boots the real `ClientApplication` against a set of
in-memory `Local*Service` fixtures, opens each window type the docs
need, and writes a PNG per capture to a configurable output directory.
It runs headless (via `MainWindow::SetHideForTesting()`) and does not
need a live server, network, or real user session.

The same harness also renders modal dialogs (LoginDialog, …) and, as
the fixture grows, popup menus and device-state variants.

It replaces hand-captured screenshots for everything tagged `auto-*`
in `client/docs/screenshots/image_manifest.json`.

## Source layout

Everything lives under `client/tools/screenshot_generator/`:

| File | Responsibility |
|---|---|
| `main.cpp` | Test fixture `ScreenshotGenerator` and the three `TEST_F`s: `CaptureAllWindows`, `CaptureMainWindow`, `CaptureDialogs`. |
| `screenshot_config.{h,cpp}` | `ScreenshotSpec`, `DialogSpec`, and `ScreenshotConfig::Load()` that parses `screenshot_data.json` and resolves `SCREENSHOT_IMAGE_MANIFEST`. |
| `screenshot_output.{h,cpp}` | `GetOutputDir()` — resolves `SCREENSHOT_OUT_DIR` once. |
| `widget_capture.{h,cpp}` | `SaveScreenshot(QWidget*, const ScreenshotSpec&)` — the generic "resize, grab, save" helper for sub-widgets inside the MDI area. |
| `graph_capture.{h,cpp}` | `SaveGraphScreenshot` builds a standalone `MetrixGraph` (hidden main windows don't lay out `QSplitter` children); `MakeGraphDefinition` builds the matching `WindowDefinition` for the profile path. |
| `dialog_capture.{h,cpp}` | `CaptureDialog(spec, env)` — per-kind builders invoke the component's public `Execute…Dialog()` factory, which calls `show()` internally; we then find the visible dialog via `QApplication::topLevelWidgets()`, grab, and hide+reject. |
| `fixture_builder.{h,cpp}` | `MakeScreenshotPage` (builds a `Page` with one window per `ScreenshotSpec`) and `MakeLocalTimedDataService` (populates a `FakeTimedDataService` from the fixture's `timed_data` array). |
| `screenshot_data.json` | The fixture: nodes, tree, timed data, events, graph config, screenshot list, dialog list. |
| `CMakeLists.txt` | Builds `screenshot_generator.exe`; links `client_qt_lib + base_unittest + graph_qt` and compiles `client_application.{cpp,h}` directly (it's excluded from `client_qt_lib`). |

The docs refresh helper for the current rollout lives outside that
tree at `client/docs/screenshots/update_screenshots.cmd`. It copies only the approved
`scada-docs/img/` batch (`client-login.png`, `client-retransmission.png`,
`graph-cursor.png`, `users.png`).

### Why the split

The prior single-file `app/screenshot_generator.cpp` pulled in every
component the fixture touched. Splitting by capture kind means:

- A change to a component's dialog API only rebuilds `dialog_capture.cpp`.
- `graph_capture.cpp` only sees the graph headers, `widget_capture.cpp`
  only sees Qt.
- Adding a new capture kind is a contained edit — a new builder in the
  right `*_capture.cpp` plus a JSON entry — instead of a surgery on the
  big test file.

## Capture pipelines

Three `TEST_F` bodies cover the three capture modes:

### `CaptureAllWindows` — main-window views

1. Reads the `screenshots:` array from the fixture.
2. Builds a `Page` with one `WindowDefinition` per entry and saves it
   into a fresh profile.
3. Calls `app_.Start()`, spins the Qt event loop 20× so async data
   loads and model updates complete.
4. For each spec, locates the matching `OpenedView` by
   `window_info().name`, resizes to the spec dims, grabs a `QPixmap`
   and saves under `GetOutputDir() / spec.filename`.
5. `Graph` is special-cased: it goes through `SaveGraphScreenshot`
   which creates a fresh `MetrixGraph` as a top-level, because the
   hidden main window's `QSplitter` doesn't lay children out.

### `CaptureMainWindow` — the whole main window at 1920×1080

Opens a couple of representative views (`EventJournal`, `Summ`,
`Struct`) in a profile, starts the app, resizes the `QMainWindow` to
match `scada-docs/img/client-window.png` and grabs the whole thing.

### `CaptureDialogs` — modal dialogs

1. Reads the `dialogs:` array.
2. For each entry, dispatches on `spec.kind` in `dialog_capture.cpp`:
   - `login` → `BuildLoginDialog` → `ExecuteLoginDialog()` which
     `show()`s the dialog non-modally (see `aui/qt/dialog_util.h`).
3. `GrabAndCloseVisibleDialog` scans `QApplication::topLevelWidgets()`
   for the visible `QDialog`, resizes to spec dims if set, grabs a
   pixmap, then calls `reject()` so the factory's `deleteLater` path
   fires.

Each dialog kind is at most 10 lines of code plus its entry in the
`if/else` chain in `CaptureDialog`.

## Why we go through public factories for dialogs

Every dialog class is declared privately in its own `.cpp` file; the
`Ui::` header is generated by AUTOUIC inside the owning module's Qt
target. The screenshot generator is not part of that target, so it
cannot include `components/login/qt/login_dialog.h` directly.

What is public is `components/<dlg>/<dlg>_dialog.h` — a free function
like `ExecuteLoginDialog(executor, context)` that internally creates
the dialog and `show()`s it. That's our entry point, and it's also
exactly the path the real client takes, so we inherit any bug fixes
there for free.

The one awkward bit is retrieving the instance back: we scan
`QApplication::topLevelWidgets()` for a visible `QDialog`. This relies
on there being at most one visible dialog at a time, which is fine for
the sequential `for` loop in `CaptureDialogs`.

## Fixture JSON schema

`screenshot_data.json` has these top-level keys:

| Key | Type | Purpose |
|---|---|---|
| `nodes` | array | Address-space entries: `{id, ns, name, class}`, class ∈ `object` \| `variable`, `base_value` on variables. Loaded into `LocalAttributeService`; the running `v1::NodeServiceImpl` pulls each node's attributes through it on demand. |
| `tree` | object | Parent → children map. Keys are `"<ns>.<id>"` or bare IDs for ns=1. Loaded into `LocalViewService`; the running `AddressSpaceFetcher` walks it via `Browse` to populate the live address space. |
| `timed_data` | array | `{formula, values}` entries — values are spaced at 30-minute intervals ending at "now". Feeds `FakeTimedDataService`. |
| `events` | array | `{id, hours_ago, severity, message, node_id, change_mask}` — injected into `LocalHistoryService`. |
| `graph` | object | Panes, lines (`path`, `color`, `pane`, `dots`, `stepped`), time scale. Used by both the `Graph` profile path and standalone rendering. |
| `screenshots` | array | `{type, filename, width, height}` — one entry per main-window view. `type` matches `WindowInfo::name`. |
| `dialogs` | array | `{kind, filename, width?, height?}` — one entry per modal dialog. `kind` is the dispatch key in `dialog_capture.cpp`. |

`width`/`height` should match the scada-docs target image pixel-exact
so the generator output is a drop-in replacement.

## Running it

Building the `screenshot_generator` target runs the generator as a
POST_BUILD step, dropping the PNGs into `client/docs/screenshots/`
(gitignored). That keeps the local gallery in lock-step with the
current client build — useful for comparing runs before publishing
anything to scada-docs.

```batch
cmake --build --preset release-dev -t screenshot_generator
```

To render into a different directory (typically `scada-docs/img/`
before committing a refresh there):

```batch
set SCREENSHOT_OUT_DIR=C:\path\to\scada-docs\img
set SCREENSHOT_IMAGE_MANIFEST=C:\tc\scada\client\docs\screenshots\image_manifest.json
build\ninja-dev\bin\RelWithDebInfo\screenshot_generator.exe
```

`SCREENSHOT_OUT_DIR` is required; the POST_BUILD step sets it for the default `client/docs/screenshots/` output. `SCREENSHOT_IMAGE_MANIFEST` is optional; if unset, the generator falls back to its built-in manifest search paths.

For the first `scada-docs` rollout, use the client-side helper script
`client/docs/screenshots/update_screenshots.cmd`:

```bash
cmd.exe /c "cd /d C:\tc\scada && cmake --build --preset release-dev --target screenshot_generator"
cmd.exe /c C:\tc\scada\client\docs\screenshots\update_screenshots.cmd
```

The script copies the currently approved rollout subset from
`client/docs/screenshots/` into `scada-docs/img/`:
`client-login.png`, `client-retransmission.png`, `graph-cursor.png`,
and `users.png`.

### Full pipeline

From the repo root (`C:\tc\scada`), the end-to-end refresh pipeline is:

```batch
cmd.exe /c "cd /d C:\tc\scada && cmake --build --preset release-dev --target screenshot_generator"
cmd.exe /c "cd /d C:\tc\scada && call client\docs\screenshots\update_screenshots.cmd"
```

What this does:

1. Rebuilds `screenshot_generator`.
2. Lets the target's POST_BUILD step regenerate `client/docs/screenshots/*`.
3. Copies the approved rollout subset into `C:\tc\scada\scada-docs\img\`.

Use the `call` form for the second command. Direct `cmd.exe /c C:\...\foo.cmd`
invocation can be parsed incorrectly by `cmd.exe` in some environments.

Run a subset with `--gtest_filter`:

```batch
screenshot_generator.exe --gtest_filter=ScreenshotGenerator.CaptureDialogs
```

## Adding a new auto-screenshot

Pick the flow that fits the new image's manifest tag.

### `auto-view` — a new main-window view

1. Register or locate the `WindowInfo` in the owning component.
2. Append `{ "type": "…", "filename": "…", "width": W, "height": H }`
   to `screenshots:` in `screenshot_data.json`. Match the
   scada-docs dimensions exactly.
3. If the view needs nodes, timed data, or events that aren't already
   in the fixture, extend the matching arrays.
4. Add an entry to `client/docs/screenshots/image_manifest.json` tagged
   `auto-view`.
5. Rebuild and verify the PNG lands.

### `auto-dialog` — a new modal dialog

1. Confirm there's a public `Execute…Dialog()` (or equivalent)
   factory; if the component only exposes the Qt-specific class,
   add a thin free-function factory in the module's
   cross-platform header.
2. Add a per-kind builder to `dialog_capture.cpp` following
   `BuildLoginDialog` as a template: construct the context, call the
   factory, return.
3. Add a new arm to the `if/else` chain in `CaptureDialog`.
4. Append `{ "kind": "…", "filename": "…", "width": W, "height": H }`
   to `dialogs:` in `screenshot_data.json`.
5. Entry in `client/docs/screenshots/image_manifest.json` tagged
   `auto-dialog`.

### `auto-menu` — a right-click popup

(Not yet implemented — this section will be filled in once the menu
rendering pipeline lands as part of the `auto-menu` task.)

### `auto-state` — a device runtime-state capture

(Not yet implemented — needs fixture knobs for per-device state, which
are the subject of the `auto-state` task.)

## Managing scada-docs/img

The source of truth for which files are auto vs manual is
`client/docs/screenshots/image_manifest.json`.

Workflow:

1. For the current rollout, run
   `client/docs/screenshots/update_screenshots.cmd`.
   It publishes only `client-login.png`, `client-retransmission.png`,
   `graph-cursor.png`, and `users.png` into `scada-docs/img/`.
2. `git diff img/` in scada-docs to review every image that changed.
   Expect a diff any time the real UI changes — that is the visual
   regression signal. If the diff is noise only (font antialiasing,
   clock values), either tighten the fixture or accept it.
3. Commit the scada-docs image changes in a separate PR from the
   client change that caused them. It's easier to review image diffs
   on their own, and it lets the client PR stay focused.
4. When removing a feature from the client, update
   `image_manifest.json` by retagging the orphaned files `obsolete` in the
   same PR that removes the feature — don't leave `auto-*` rows
   pointing at dead window types.

## Why no native window frames

Screenshots intentionally don't include the OS title bar / border.
The capture runs headless: `MainWindow::SetHideForTesting()` makes
the main window skip `show()`, dialogs are shown but never mapped to
the screen compositor, and everything is captured via
`QWidget::grab()` which renders the widget tree into a QPixmap
without touching the OS window manager.

This keeps the suite portable (no desktop session required on
Windows or Linux CI) at the cost of frame-less output. We evaluated
two alternatives and rejected both:

- **`QScreen::grabWindow(winId)`** — on Qt 5 / Windows this calls
  `PrintWindow` without `PW_RENDERFULLCONTENT` and returns only the
  client area. On top of that, grabbing a desktop rect at the
  window's `frameGeometry()` picks up whatever is on top in OS
  Z-order, which on a busy dev box is rarely our window.
- **`PrintWindow(hwnd, hdc, PW_RENDERFULLCONTENT)`** — captures
  frame + client independently of Z-order, but doesn't reliably
  refresh DWM's internal bitmap for Qt dialogs that were never
  foregrounded, so child widgets come out black even though
  `widget->grab()` on the same dialog renders them fine. The
  write-dialog family hit this consistently.

If docs ever need authentic OS frames, the cleanest path is to
composite `widget->grab()` onto a PrintWindow capture — frame from
PrintWindow, content from the widget tree render. Not worth it right
now.

## Known constraints

- **QApplication argv is zeroed.** `AppEnvironment` builds the
  `QApplication` with `{0, nullptr}`, so command-line flags
  (`--out`, …) are invisible to `QCoreApplication::arguments()`.
  Configuration has to flow through env vars instead.
- **`StubTransportFactory` is out of sync.** The upstream
  `third_party/net` stub doesn't match the current
  `TransportFactory` interface. `dialog_capture.cpp` carries its own
  one-method `NullTransportFactory` for now.
- **Registry reads on LoginDialog.** `LoginController` reads the
  saved user list from `HKEY_CURRENT_USER\Software\Telecontrol\Workplace`.
  On a fresh dev box the form renders empty, which happens to match
  the docs image. If that changes, the fixture will need to shim the
  registry (or the controller will need a test hook).
- **One visible dialog at a time.** `GrabAndCloseVisibleDialog`
  assumes at most one visible `QDialog` when it scans the top-level
  widgets. That holds inside the sequential `for` loop but would
  break if the generator ever rendered dialogs concurrently.
- **Russian translator + Fusion style are installed directly** in the
  fixture constructor, bypassing `InstalledTranslation` and
  `InstalledStyle`. Both normally round-trip through `QSettings`,
  which is unreliable here because `AppEnvironment` builds a bare
  `QApplication`, and `InstalledStyle` also writes back on
  destruction, which would let a second `TEST_F` pick up the wrong
  style name.
