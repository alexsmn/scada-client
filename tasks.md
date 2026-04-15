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

### Remove `client_qt_lib` and make modules reference each other

Goal: stop using the catch-all `client_qt_lib` umbrella and make each
binary link only the modules it actually needs. That means expressing
the real dependency graph between client modules directly, so
`screenshot_generator`, `client_qt`, test binaries, and small tools can
all build against minimal link sets instead of dragging the full UI
stack transitively.

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
| Window types not yet captured | ~10 | `client-login.png`, `client-retransmission.png`, `users.png`, `limits.png`, `limits-chart.png`, `graph-cursor.png`, `display.png` |
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

1. **Inventory and tag.** Walk `scada-docs/img/*` and maintain a
   manifest at `client/docs/screenshots/image_manifest.json` tagging each
   file with one of `auto-view`, `auto-dialog`, `auto-menu`,
   `auto-state`, `manual-diagram`, `manual-modus`, `manual-os`, or
   `obsolete`. The manifest is the source of truth for what the
   generator must produce; everything tagged `manual-*` stays
   hand-maintained.

2. **Match existing generator output.** For the items currently
   tagged `auto-view` *and* already produced by the generator (just
   `client-window.png` today, plus near-misses like `graph-cursor.png`
   ↔ `graph.png`), align filenames, dimensions, and crop boxes so
   the generator's output is a drop-in replacement. Update the
   generator's `screenshot_data.json` `screenshots:` section to
   emit the scada-docs filename instead of the current short name.

3. **Add an output-directory flag.** Extend
   `screenshot_generator.cpp` with a `--out <dir>` (or
   `SCREENSHOT_OUT_DIR` env var) so the generator can write to an
   explicit target directory instead of relying on an implicit default.

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

    Blocked on the **content-gap** tasks below: every candidate
    compared so far against the scada-docs originals loses
    information (empty tables, missing values, formulas in place of
    display names, no status-bar content, …). Land the fixture
    enrichment tasks first, then replace one subset of images per
    PR.

### Screenshot-generator content gaps

Each gap below is one small fixture / rendering change that unblocks
one or more `auto-*` images for bulk replace (subtask 13). Discovered
by side-by-side diffing the current generator output against
`scada-docs/img/`.

#### Gap: seed login user list + expanded combo state

`scada-docs/img/client-login.png` shows an OPEN combobox with 7 user
names (root, Диспетчер 1, Инженер АСУ, Диспетчер, Руководитель,
Инженер, АСУ). Our capture shows `root` only in a closed combo.
Needs: (1) seed `LoginController::user_list` — currently read from
`HKEY_CURRENT_USER\Software\Telecontrol\Workplace\UserList` in
`login_controller.cpp` — either via registry write in the fixture
setup or by adding a test hook that accepts a pre-populated list;
(2) optionally trigger `QComboBox::showPopup()` before capture so the
list is visible.

#### Gap: populate analog limits + node display name in `LimitDialog`

`scada-docs/img/limits.png` shows "Температура нагрева" as source
title and four populated limit values (Аварийные: Верхняя 90 /
Нижняя −25; Уставки: Верхняя 70 / Нижняя −10). Our capture shows
"Активная мощность" + four empty fields. Also blocks
`limits-chart.png` (subtask 4). Needs: extend the
`LocalNodeService` / `LocalAttributeService` fixture path so a node
can expose `AnalogItemType_LimitLo/Hi/LoLo/HiHi` as `HasProperty`
children with values. Then add a node modelled as
"Температура нагрева" and point the limits dialog at it.

#### Gap: `WriteDialog` source title + current value + unit

`scada-docs/img/ti-manual-control.png` and
`ti-remote-control-enabled.png` show: source title
"Температура нагрева", current value "41 °C", new value pre-filled
with "25", engineering unit "°C" next to the combo. Our captures
show `TS.200` (formula, not display name) and empty current / new /
unit fields. Needs: (1) confirm `TimedDataSpec::GetTitle()` returns
the node's `display_name` when wired to the local fixture — should
already work, verify; (2) `FakeTimedDataService` must return a
current `DataValue` for formulas like `TS.200` so
`GetCurrentValue()` renders; (3) populate
`AnalogItemType_EngineeringUnits` on the fixture node so
`GetAnalogUnits()` returns "°C".

#### Gap: `OutputCondition` for `ti-remote-control-enabled`

`scada-docs/img/ti-remote-control-enabled.png` shows a
"Условие: Выполнено" row between current and new value. Our
`write-remote` capture has no condition row because the fixture node
has no `DataItemType_OutputCondition`. Needs: populate
`OutputCondition` on the TS/TI node and wire the referenced formula
to always evaluate truthy so `has_condition_=true` AND
`IsConditionOk()=true`. Also unblocks `ti-remote-control-disabled`
(same node, condition evaluates false) and
`ts-remote-control-confirm` (post-second-stage confirmation).

#### Gap: Users table — populated rows

`scada-docs/img/users.png` shows 13 rows populated with Browse Name
(`SCADA.234`, `USER.1`..`USER.12`), Name (root, Клиент 1..10,
Администратор, guest), Права (0..3), and Множество сессий (Да /
Нет). Our capture shows two default columns (Browse Name, Name) and
no rows. Needs: add user nodes under the Users folder (`7.29`) in
the fixture tree, each with the scada-specific attributes the Users
table model reads (likely a `security::UserType` node with
properties for rights and multi-session). Find the read path in
`components/node_table/` and work backwards.

#### Gap: `Transmission` view — populated rows

`scada-docs/img/client-retransmission.png` shows a Transmission view
with 4 rows: `TC1` (1001), `TC8` (1002), `Ua` (2001),
"Дорасчет Ua.среднее" (2002). Our capture shows column headers
"Сигнал" / "Адрес" and no rows. Needs: add transmission node
children under the relevant retransmission folder (`7.34` /
`7.342` / `7.343` / `7.344`) with the address attribute populated.
Check `TransmissionModel` / `TransmissionView` to see which
attributes it reads.

#### Gap: graph cursor + 2-pane noisy variant

`scada-docs/img/graph-cursor.png` shows a 2-pane graph with noisy
data and two vertical cursor lines at specific timestamps (the
"cursor" feature from `client/graph.md`). Our `graph-cursor.png`
shows 3 panes with smooth sinusoidal data and no cursor. Needs:
(1) switch the generator's graph fixture to a 2-pane layout;
(2) denser / noisier values in `timed_data`; (3) enable `MetrixGraph`
cursor(s) before `SaveGraphScreenshot`. Lowest priority — our
output is a working graph, just different content.

#### Gap: `client-window` page layout — Modus schematic centre

`scada-docs/img/client-window.png` shows the main window with a
Modus schematic (ОРУ-110 кВ, КРУ-10 кВ, ЗРУ-6 кВ) as the central
page — a substation mimic with coloured switches, transformers,
buses, and a properties panel on the right. Our `CaptureMainWindow`
uses an `EventJournal + Summ + Struct` layout. Two options:
(a) swap the fixture page to Modus — blocked on the fake Modus
runtime (out of scope); (b) retag `client-window.png` as
`manual-modus` (or similar) until the fake runtime exists, and
accept that the auto-generated composite isn't the one the docs
show.

#### Gap: some `auto-menu` images are actually annotated view screenshots

`scada-docs/img/menu-events.png` (despite the name) shows an Event
view with coloured row highlighting and red-underline annotations
on status-bar elements — not a right-click popup menu. Several
other `menu-*.png` may be similar. Audit all 15 `auto-menu` rows in
`image_manifest.json`, reclassify the ones that are annotated view
composites to `manual-annotated`, and let subtask 7 (menu
rendering) focus only on true `QMenu` popups.

#### Gap: fixture needs realistic event log rows + status-bar content

`scada-docs/img/menu-events.png` and similar show canonical event
rows ("Связь с сервером установлена...", "Разрыв связи с сервером
10.0.1.166..."), timestamps like `07.02.2024 09:28:53.130`,
importance 50 / 60, object "Локальное событие", and status-bar
strings "Готово", "Диспетчер 1", "Подключен", "Отклик: 0 мс". Our
`events[]` array is plausible but doesn't match the docs' canonical
examples; `events-log.png` rendered from our fixture drifts from
the narrative the docs describe. Needs: (1) refresh `events[]` with
the canonical messages; (2) wire the status bar to render populated
user / connection / response-time strings instead of the default
placeholders.

#### Gap: Object tree fixture needs device / section structure

Most docs screenshots that include the Object tree
(`client-retransmission.png`, `devices-on.png`, `client-window.png`,
`menu-create-object*`) show a structured tree: M340, Smart Termo,
Диагностика, КРУ, КСВ2.0, Мониторинг t, ЭНИП-2 + ЭНМВ-1,
ЭСТРА-ПС (ТИ/ТС/ТУ), with expandable nodes like TS1..TC8, F, T,
Ua/Ub/Uc, Uab/Ubc/Uca, etc. Our tree has "Подстанция Альфа/Бета/
Гамма" placeholders. Either align our fixture with the docs'
canonical tree OR retag the affected docs images and accept the
mismatch. Cascades into every view-type screenshot that includes
the Object tree.

## Misc

### Add more shutdown logs

### Custom table: Open new table for multiple selection

### Get rid of `IsWorking`
