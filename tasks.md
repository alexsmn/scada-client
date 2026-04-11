# Client tasks

Work that lives **only** in `scada-client` (UI, components, profile,
modus, vidicon). Cross-cutting items that span the client and server (or
that touch the wire protocol) belong in [`tasks.md`](../tasks.md) at the
repo root.

Tasks are grouped by area. New entries go under the matching cluster or,
if none fits, into **Misc** — never as a flat top-level entry.

## Object, hardware & file trees

### `ObjectView`: Don't show "has children" for TS and TIT

### `ConfigurationNodeView`: Show "loading" when node itself is not loaded

### `HardwareTree`: Metrics from root

### Save expanded tree nodes

### Locate current object in the `ObjectTree`

### `ObjectTree`: Search / filter

### Progress bar for object pulls for `WindowDefinition` building

## Property editor, limits & dialogs

### `PropertyView`: Expand on the first open. Especially for just created nodes

### Don't open duplicate node-properties window

### `LimitDialog`: Show error when set limits fails and don't close the dialog

### Property editor: Non-modifiable attributes must be grey

## Command framework

### Split `SelectionCommands` into multiple files or per components

### Split `OpenedViewCommands` into multiple files or per components

### `MainCommands`, `OpenedViewCommands` and `SelectionCommands` should be singletons

Then they can be constructed via `ComponentApi`.

### Apply dependency-injection framework

### Isolate components and make them responsible for wiring

## Login & data services

### Login: Make server list depend on type

Needed for OPC UA connection.

### `DeviceCommands`: Use aliases where possible

Debatable.

### Don't create `MonitoredItem` on selection

E.g. for files selected in File View.

## Events, graphs & display

### Show local error events in event view in red color

### Graph: A line mark for historical vs real-time points

### Icons or hints for value indicators like `[C]`

## Modus & data-item editor

### Modus Qt: Implement Modus commands

### `DataItem`: Address builder

## Screenshot generator

### Extend `screenshot_generator` to regenerate all scada-docs screenshots

Goal: bring `scada-docs/img/*` under the offline screenshot generator
(`app/screenshot_generator.cpp`) so doc images can be regenerated
reproducibly instead of being captured by hand. Today the generator
renders ~17 window-type PNGs into `scada/screenshots/`; `scada-docs/img/`
has ~58 images, of which only `client-window.png` overlaps by name. The
rest were captured manually years ago and have drifted from the live UI
(different style, English labels, mismatched theme).

Approximate inventory (50 images referenced from `scada-docs/*.md`):

| Category | Count | Example filenames |
|---|---|---|
| Already produced by the generator | 1 | `client-window.png` |
| Window types not yet captured | ~10 | `client-login.png`, `client-retransmission.png`, `users.png`, `limits.png`, `limits-chart.png`, `graph-cursor.jpg`, `display.png` |
| Modal dialogs | ~8 | `ts-manual-control.png`, `ti-remote-control-confirm.png`, `ts-remote-control-{enabled,disabled}.png`, etc. |
| Right-click / popup menus | ~13 | `menu-create-object*.png`, `menu-parameters*.png`, `menu-events*.png`, … |
| Device runtime-state captures | 5 | `devices-{create,off,on,status,traffic}.png` |
| Data-item parameter captures | ~12 | `ti-channel.png`, `ti-formula.png`, `ti-element-parameters.png`, `ts-channel.png`, … |
| Modus schematics | 4 | `display.png`, `modus-f11-breaker.png`, `modus-f11-bus.png`, `modus-scheme.png` |
| Diagrams (not screenshots) | 4 | `MODBUS_func.png`, `iec-60870-5-transport.png`, `iec-61850-model.png`, `structure.png` |
| OS-level (Windows firewall) | 2 | `firewall.png`, `firewall.jpg` |

The diagram and OS-level images are out of scope — they will stay as
hand-maintained assets. Modus screenshots are out of scope until the
test fixture supports a fake Modus runtime.

**Incremental subtasks** (each is small enough to land as its own
PR / commit):

1. **Inventory and tag.** Walk `scada-docs/img/*` and write a
   manifest at `scada-docs/img/MANIFEST.md` (or `.yaml`) tagging each
   file with one of `auto-view`, `auto-dialog`, `auto-menu`,
   `auto-state`, `manual-diagram`, `manual-modus`, `manual-os`, or
   `obsolete`. The manifest is the source of truth for what the
   generator must produce; everything tagged `manual-*` stays
   hand-maintained.

2. **Match existing generator output.** For the items currently
   tagged `auto-view` *and* already produced by the generator (just
   `client-window.png` today, plus near-misses like `graph-cursor.jpg`
   ↔ `graph.png`), align filenames, dimensions, and crop boxes so
   the generator's output is a drop-in replacement. Update the
   generator's `screenshot_data.json` `screenshots:` section to
   emit the scada-docs filename instead of the current short name.

3. **Add an output-directory flag.** Extend
   `screenshot_generator.cpp` with a `--out <dir>` (or
   `SCREENSHOT_OUT_DIR` env var) so the generator can write directly
   into `scada-docs/img/` from a docs-side script, instead of always
   writing into `scada/screenshots/`.

4. **Add the missing window-type captures** (~10 items). For each
   new window type — `Login`, `Transmission`, `Users` table,
   `Limits`, etc. — add a `WindowInfo` line to
   `screenshot_data.json` and verify it renders. Most of these
   already have a registered `WindowInfo`; only the fixture data
   and screenshot dimensions need to be added.

5. **Localise the fixture data.** Make sure the rendered screenshots
   match the Russian content of the docs (device names, event
   messages, user names) so they look native. The current fixture
   already uses Russian for many strings — finish the gaps.

6. **Add modal-dialog rendering.** Extend the generator with a
   `dialogs:` section in `screenshot_data.json` that lists
   dialog classes to instantiate (`LimitDialog`, `WriteDialog`,
   `LoginDialog`, `MultiCreateDialog`, `ChangePasswordDialog`,
   `NodePropertiesDialog`, …) and renders each one offscreen.
   Cover the ~8 modal dialogs the docs need.

7. **Add popup-menu rendering** (~13 items). Trickier than dialogs:
   menus are normally tied to a `QMenu::popup()` call from a
   real event. Either (a) build a small helper that constructs the
   `QMenu` from the existing `SelectionCommandRegistry` /
   `GlobalCommandRegistry` and renders it standalone, or (b) drop
   the menu screenshots from the docs and replace them with text
   call-outs ("right-click → Create → TS/TI"). Decide before
   investing in (a).

8. **Device state captures** (~5 items). Extend the screenshot
   generator's local services with the ability to set per-device
   state (online / offline / suspended / freshly-created / busy),
   then capture the `Devices` window in each state. This needs new
   fixture knobs in `screenshot_data.json`.

9. **Data-item parameter captures** (~12 items). Most of these are
   the per-channel / per-formula / per-control sub-tabs of the
   parameters dialog. They depend on subtask 6 (modal-dialog
   rendering) plus a way to switch the dialog's active tab from
   the fixture.

10. **Hook into the docs build.** Add a `make screenshots` target
    (or a `bundle exec rake screenshots` task) inside scada-docs
    that invokes the generator with `--out img/` and verifies the
    diff is non-empty if the generator changed. This is the
    docs-side entry point operators use.

11. **CI integration.** Run the generator on every PR that touches
    `components/`, `aui/`, `main_window/`, or `app/qt/`, diff the
    output against `scada-docs/img/`, and fail the build if there's
    an unexpected drift. Either auto-commit the regenerated images
    to scada-docs in a follow-up PR or surface the diff as a CI
    artifact for human review.

12. **Document the workflow.** Add a "Regenerating screenshots"
    section to `docs/design.md` §FR-21 and to scada-docs `CLAUDE.md`
    explaining how to run the generator, where the manifest lives,
    what's auto vs. manual, and how to add a new auto-screenshot.

13. **Bulk replace** the existing legacy images in
    `scada-docs/img/` with the auto-generated ones in batches by
    category — view types first (lowest risk), then dialogs, then
    states. Each batch is one PR with side-by-side before/after
    review.

## Misc

### Add more shutdown logs

### Custom table: Open new table for multiple selection

### Get rid of `IsWorking`
