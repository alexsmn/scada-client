# Screenshot generator

Low-level design of the offline screenshot generator plus the
day-to-day workflow for managing images under `client/docs/screenshots/`.

For the high-level "what is FR-21", see `requirements.md` §3. For the
regeneration workflow, see `design.md` §3.1 "Regenerating the doc
screenshots". This doc is the authoritative reference when you are modifying
the generator itself or adding new captures.

## What the generator is

`client/tools/screenshot_generator/client_screenshot_generator.exe` is a Qt
test binary that boots the real `ClientApplication` against a set of
in-memory `Local*Service` fixtures, opens each window type the docs
need, and writes a PNG per capture to a configurable output directory.
It runs headless (via `MainWindow::SetHideForTesting()`) and does not
need a live server, network, or real user session.

The same harness also renders modal dialogs (LoginDialog, …) and, as
the fixture grows, popup menus and device-state variants.

It replaces hand-captured screenshots for everything tagged `auto-*`
in `client/docs/screenshots/image_manifest.json`.

## Inputs and outputs

### Command-line flags

The generator reads these command-line flags:

| Flag | Required | Purpose |
|---|---|---|
| `--out=<dir>` | yes | Output directory for generated PNGs. The CMake `POST_BUILD` step sets this to `client/docs/screenshots/`. |
| `--image-manifest=<path>` | no | Override path to `image_manifest.json`. If unset, the generator falls back to its built-in manifest search paths. |
| `--only=<files>` | no | Comma / semicolon / newline-separated list of PNG filenames to regenerate, for example `client-login.png,users.png`. If unset, the generator renders every managed screenshot. |

### Outputs

The generator writes PNGs into the directory passed via `--out`.

Current output groups:

| Source | Output files |
|---|---|
| `CaptureAllWindows` | One PNG per `screenshots:` entry from `screenshot_data.json`, filtered by the image manifest and `--only`. |
| `CaptureMainWindow` | `client-window.png` |
| `CaptureDialogs` | One PNG per `dialogs:` entry from `screenshot_data.json`, filtered by the image manifest and `--only`. |

The `update-screenshots-dev` workflow preset refreshes the approved local subset in `client/docs/screenshots/`.

## Source layout

Everything lives under `client/tools/screenshot_generator/`:

| File | Responsibility |
|---|---|
| `main.cpp` | Test fixture `ScreenshotGenerator` and the three `TEST_F`s: `CaptureAllWindows`, `CaptureMainWindow`, `CaptureDialogs`. |
| `screenshot_config.{h,cpp}` | `ScreenshotSpec`, `DialogSpec`, and `ScreenshotConfig::Load()` that parses `screenshot_data.json`, resolves `--image-manifest`, and applies `--only`. |
| `screenshot_options.{h,cpp}` | Parses `--out`, `--image-manifest`, and `--only` once from the process command line. |
| `screenshot_output.{h,cpp}` | `GetOutputDir()` — resolves `--out`. |
| `screenshot_wait.{h,cpp}` | Shared Qt event-loop promise waits used by window/dialog capture and covered by `client_qt_unittests`. |
| `widget_capture.{h,cpp}` | `SaveScreenshot(QWidget*, const ScreenshotSpec&)` — the generic "resize, grab, save" helper for sub-widgets inside the MDI area. |
| `graph_capture.{h,cpp}` | `SaveGraphScreenshot` builds a standalone `MetrixGraph` (hidden main windows don't lay out `QSplitter` children); `MakeGraphDefinition` builds the matching `WindowDefinition` for the profile path. |
| `dialog_capture.{h,cpp}` | `CaptureDialog(spec, env)` — per-kind builders invoke the component's public `Execute…Dialog()` factory, which calls `show()` internally; we then find the visible dialog via `QApplication::topLevelWidgets()`, grab, and hide+reject. |
| `fixture_builder.{h,cpp}` | `MakeScreenshotPage` (builds a `Page` with one window per `ScreenshotSpec`) and `MakeLocalTimedDataService` (populates a `FakeTimedDataService` from the fixture's `timed_data` array). |
| `screenshot_data.json` | The fixture: nodes, tree, timed data, events, graph config, screenshot list, dialog list. |
| `CMakeLists.txt` | Builds `client_screenshot_generator.exe`; links `client_qt_core + client_export_csv_qt + client_favorites_qt + client_main_window_qt + client_modus_qt + client_portfolio_qt + client_print_service_qt + client_properties_qt + client_vidicon_qt + scada_core_opcua + base_unittest + graph_qt` and compiles `client_application.{cpp,h}` directly (it's excluded from `client_qt_core`). |

The docs refresh helper for the current rollout lives outside that
tree at `cmake/update_screenshots.cmake`, driven by the
`update-screenshots-dev` workflow preset. It refreshes only the approved
local batch (`client-login.png`, `client-retransmission.png`,
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
`Struct`) in a profile, temporarily disables `SetHideForTesting()` for
that test, waits for the `Struct` tree to materialize child rows,
shows the real `QMainWindow`, activates its layouts, and grabs the
window at 1920×1080.

The key finding from debugging was that hidden top-level capture was
too early: `ConfigurationTreeModel` logged that the `Struct` children
were loaded (`Load children` / `Children` for `SCADA.24`), but the
hidden-window screenshot still rendered only the single top-level
`Все объекты` row. Showing the real window is now the intended fix for
`client-window.png`; the standalone-graph workaround remains specific
to the graph capture path.

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

`width`/`height` should match the intended docs image pixel-exact so
the generator output stays stable.

## Running it

Building the `client_screenshot_generator` target runs the generator as a
POST_BUILD step, dropping the PNGs into `client/docs/screenshots/`
(gitignored). That keeps the local gallery in lock-step with the
current client build.

```batch
cmake --build --preset release-dev -t client_screenshot_generator
```

To render into a different directory:

```batch
build\ninja-dev\bin\RelWithDebInfo\client_screenshot_generator.exe ^
  --out=C:\path\to\output-dir ^
  --image-manifest=C:\tc\scada\client\docs\screenshots\image_manifest.json
```

`--out` is required; the POST_BUILD step sets it for the default `client/docs/screenshots/` output. `--image-manifest` is optional; if unset, the generator falls back to its built-in manifest search paths.

To regenerate only a few named files:

```batch
build\ninja-dev\bin\RelWithDebInfo\client_screenshot_generator.exe ^
  --out=C:\tc\scada\client\docs\screenshots ^
  --image-manifest=C:\tc\scada\client\docs\screenshots\image_manifest.json ^
  --only=client-login.png;users.png
```

To refresh the approved local subset, use the workflow preset:

```bash
cmd.exe /c "cd /d C:\tc\scada && cmake --workflow --preset update-screenshots-dev"
```

The target rebuilds `client_screenshot_generator` if needed, then refreshes
the currently approved local subset in `client/docs/screenshots/`:
`client-login.png`, `client-retransmission.png`, `graph-cursor.png`,
and `users.png`.

### Full pipeline

From the repo root (`C:\tc\scada`), the end-to-end refresh pipeline is:

```batch
cmd.exe /c "cd /d C:\tc\scada && cmake --workflow --preset update-screenshots-dev"
```

What this does:

1. Rebuilds `client_screenshot_generator`.
2. Lets the target's POST_BUILD step regenerate `client/docs/screenshots/*`.

Run a subset with `--gtest_filter`:

```batch
client_screenshot_generator.exe --gtest_filter=ScreenshotGenerator.CaptureDialogs
```

## Adding a new auto-screenshot

Pick the flow that fits the new image's manifest tag.

### `auto-view` — a new main-window view

1. Register or locate the `WindowInfo` in the owning component.
2. Append `{ "type": "…", "filename": "…", "width": W, "height": H }`
   to `screenshots:` in `screenshot_data.json`. Match the
   intended docs dimensions exactly.
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

## Managing Generated Images

The source of truth for which files are auto vs manual is
`client/docs/screenshots/image_manifest.json`.

Workflow:

1. For the current rollout, run
   `cmake --workflow --preset update-screenshots-dev`.
   It refreshes only `client-login.png`, `client-retransmission.png`,
   `graph-cursor.png`, and `users.png` in `client/docs/screenshots/`.
2. Inspect `client/docs/screenshots/` to review every image that changed.
   Expect a diff any time the real UI changes — that is the visual
   regression signal. If the diff is noise only (font antialiasing,
   clock values), either tighten the fixture or accept it.
3. If you changed the fixture or capture code, also inspect any PNGs
   outside the approved rollout subset that were regenerated locally.
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

- **Qt app argv is still zeroed.** `AppEnvironment` builds the
  `QApplication` with `{0, nullptr}`, so
  `QCoreApplication::arguments()` cannot see the generator flags.
  `screenshot_options.cpp` works around that by parsing the process
  command line directly before the fixture starts.
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
