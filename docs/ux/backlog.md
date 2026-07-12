# SCADA Client — UX Implementation Backlog

> Status: living plan. A **catalogue of the reshell work** derived from
> [`shell.md`](shell.md), grouped for reference — **not a fixed waterfall.**
> Each item names the primary code it touches, its dependency, and a "done when"
> acceptance line.

## How to use this (read first)

Per `client/CLAUDE.md` → "UX implementation approach": build the reshell as
**incremental vertical slices**, not by executing P0→P5 top to bottom. Pick one
coherent surface, take it end-to-end (behind the opt-in theming flag), validate
it against real Qt widgets, ship, then re-evaluate. The groups below are a map
of *what* exists to do and how items depend on each other; the *order* is chosen
per slice with the user, not dictated by the numbering. Theming is **opt-in and
palette-first** — never made unconditional here.

The groups (P0 foundations, P1 shell chrome, P2 operator core, P3 dialog
theming, P4 Explorer/Inspector, P5 engineering) capture dependencies, not a
required sequence.

---

## P0 — Foundations (design system in code)

| # | Item | Touches | Done when |
|---|---|---|---|
| 0.1 | **Token layer** — ✅ *landed (opt-in)*: `aui/qt/theme_qt.{h,cpp}` (`scada::aui`) encodes the light/dark/high-contrast token tables, builds a Fusion `QPalette`, and generates the QSS; `ApplyTheme(theme, scope)` installs palette (+stylesheet for `kFull`). Wired in `app/qt/main.cpp` **behind the `Ux/Experimental` QSetting (off by default)**; `Ux/Theme` picks the variant, `Ux/StyleSheet=false` → palette-only. *Remaining:* drive the choice from `Profile` + a runtime switcher; validate `kFull` against ActiveX/custom-painted widgets before defaulting on. | `app/qt/`, `profile/`, `aui/qt/theme_qt.*` | Opt-in themes switch at runtime; no component hard-codes hex; values match [`design-language.md`](design-language.md). |
| 0.2 | **Severity palette single-source** — ✅ *landed (opt-in)*: `aui/severity_colors.{h,cpp}` (`scada::aui::EventRowColorsFor`) is the one theme-aware map for event/alarm colours; the event journal now resolves its row colours there instead of hardcoded RGB, so a token change restyles it (and future severity surfaces) at once. `SetSeverityTheme` follows the palette opt-in in `app/qt/main.cpp` (`kLegacy` = unchanged default). Unit-tested + validated on the real journal via `--theme=dark`. *Remaining:* wire the tree/inspector/status severity cues as those surfaces gain colouring. | `aui/`, `modules/events/` | Changing a severity token restyles every surface at once. |
| 0.3 | **Mono numerals**: apply `--font-mono` to all value/timestamp/limit cells. | `aui/models/`, table/tree delegates | Values/timestamps render tabular; columns don't jitter on update. |
| 0.4 | **Themed control primitives**: `Field`, `Combo`, `Checkbox`, `Button` (primary/default/danger/disabled+reason). | new `aui/qt/controls` | Primitives available; retire native `QInputDialog`/`QMessageBox` defaults. |

## P1 — Shell chrome

| # | Item | Touches | Dep | Done when |
|---|---|---|---|---|
| 1.1 | **Activity bar** — ✅ *landed (opt-in)*: a full-height charcoal left rail (`main_window/activity_bar_qt.{h,cpp}`, gated on the active UX theme) with the display-hierarchy sections — Overview, Alarms, Trends, Substations, Tables on top; Administration, Settings pinned at the bottom. Each live section opens its view via `FindWindowInfoByName` → `OpenView` (Alarms→EventJournal, Trends→Graph, Substations→Modus, Tables→Table); sections without a registered view (Overview/Administration/Settings today) are shown disabled. The active section carries an accent marker; the Alarms section carries an unacknowledged-count badge fed by a new `StatusBarModel::GetAlarmCount()` (wired from `EventStatusProvider`, the same source the status strip uses). Section icons reuse each view command's image where present, falling back to a letter glyph. Unit-tested (`activity_bar_unittest.cpp`: click-activates, disabled-gating, active-section, badge; plus the count accessor). Validated on the real window (`--theme=dark`). *Remaining:* back sections with `GlobalCommandRegistry` commands (so they also appear in the palette); dedicated rail icons; wire Overview/Administration/Settings once those pages exist. | new `main_window/qt/activity_bar`, `core/GlobalCommandRegistry` | 0.1 | Sections switch; badge tracks unacknowledged count. |
| 1.2 | **Top context bar** — ✅ *landed (opt-in)*: a full-width bar above the command toolbar with a brand lockup, a command/search field (Russian placeholder via `.ts`), and a live context cluster mirroring the status-bar model panes. Built in `main_window_qt.cpp::CreateContextBar`, gated on the active UX theme (so both the app and the headless generator show it). Validated on the real main window (`--theme=dark`). *Remaining:* wire the field to the command palette (1.3), and curate the cluster to plant/server/connection/user (currently mirrors all status panes). | `main_window/`, `core/` | 0.1 | Context cluster visible; field opens palette (1.3). |
| 1.3 | **Command palette** — ✅ *done (commands + views + type-to-search)*: `Ctrl K`, clicking the command-search field, **or typing a printable key in it** opens a modal palette (`main_window/command_palette_qt.{h,cpp}`) seeded with that character; it filters every registered command by title — case- and Cyrillic-fold-insensitive matching, prefix-before-interior ranking, in the Qt-free `command_match.{h,cpp}` (unit-tested) — and runs the selection through the existing handler resolution. **Views are covered**: the `Open …` view actions (Graph/Display/Table/…) are registered `CATEGORY_OPEN` commands, so they appear in the palette and their global handler opens the view. Validated on the real widget (generator capture) + unit-tested (incl. preset-filter). Address-space **tags** are split out to 1.3a. | `core/`, `controller/` | 1.2 | Fuzzy search finds commands + views; type-to-search; Enter activates. |
| 1.3a | **Palette address-space tags** — ✅ *landed (opt-in)*: data-item tags appear in the command palette and open in a table view when chosen. `NodeService` is now threaded into `MainWindowContext` (optional `NodeService*`, null in minimal/test contexts). `main_window/tag_search_index.{h,cpp}` does a **bounded, cycle-safe background browse** (from `ObjectsFolder` over `Organizes`, collecting `DataItemType` leaves, capped at 2000) kicked off at main-window construction; the palette reads the cached tags and filters them locally via the same matcher as commands, through a new `CommandPalette::ExtraItem` channel (matched/ranked like commands, `activate` runs on Enter). Unit-tested (palette extra-item match + activation). *Remaining/known limits:* the browse is a one-time cache (no incremental refresh, capped — logs nothing when capped yet); activation opens a table (no reveal-in-Explorer); interactive end-to-end validation needs a logged-in server session. | `main_window/`, `common/node_service/` | 1.3 | Typing finds tags by name; Enter opens the node. |
| 1.4 | **Editor-style workspace tabs** — ✅ *landed (opt-in)*: the central workspace `DockTabWidget` (already `documentMode`) is restyled through the theme QSS (`aui/qt/theme_qt.cpp`) to read as editor tabs — flat inactive tabs on the elevated bar, the active tab dropping its separators to blend into the content pane with an accent top marker, hover state, and a hover-highlighted close button; dock panels get a slim themed title strip in place of the native OS title chrome. No `view_manager_qt` submodule edit — pure token-driven styling, so it only applies under the reshell theme. `OpenedView` labels intact; close/reorder unchanged. Validated on the real main window (`--theme=dark` capture). *Remaining:* a custom close glyph (the native red × still shows) and tab-overflow affordance. | `aui/qt/`, `ViewManager` | 0.1 | Tabs restyled; `OpenedView` labels intact; close/reorder works. |
| 1.5 | **Status strip** — ✅ *done*: highest-severity alarm cell (coloured from the severity single source), plus the **user·role** cell (the user pane now shows `<user> · <role>`, role derived from session privileges via the pure `UserRoleKey` — Configure⇒Administrator, Control⇒Operator, else Observer) and an **endpoint/build** cell (`SessionStatusProvider::GetEndpointText` → `<host> · v<version>`, version from the module's `CLIENT_BUILD_VERSION` compile def = `${PROJECT_VERSION}`). Unit-tested (`SeverityColorsTest`, `UserRoleKeyTest`); validated on the real strip (`--theme=dark`): "Администратор", "local · v2.6.0". Russian role labels in `client_ru.ts`. | `main_window/status_bar/`, `aui/` | 0.2 | Alarm summary + user·role + endpoint/build live. |

## P2 — Operator core (the cockpit)

| # | Item | Touches | Dep | Done when |
|---|---|---|---|---|
| 2.1 | **Overview page** in default `Profile`: KPI severity strip + dominant trend + active-alarm table. | `profile/`, `modules/graph/`, `modules/events/` | 1.1–1.5 | Fresh profile lands on Overview; all three regions live. |
| 2.2 | **Active-alarm surface vs. event journal split** (principle §3) — 🔶 *in progress*. Discovery: the split already exists at the model layer — `CurrentEventModel::events()` returns `unacked_events()`, so the docked "Event" panel + EventJournal "Current" mode already show only actionable (unacknowledged) alarms, while historical mode is the full journal; severity filter + period (time range) + `ID_ACKNOWLEDGE_ALL` all present. Added the mockup's **"Unacknowledged only"** control at the model layer: `EventTableModel::SetUnacknowledgedOnly` filters the *historical* journal to actionable events via `IsEventShown` (unit-tested — `EventTableModelUnacknowledgedFilterTest`, exercised synchronously through `HistoricalEventModel::refilter_now`). *Remaining:* surface the toggle in the UI — the event context menu is `IDR_EVENT_POPUP`, a Windows resource not editable/validatable in this tree, so a cross-platform surfacing (reshell control) is needed; plus the **Area** filter (severity/period exist) and a visually distinct alarm surface per `event-journal.html`. | `modules/events/` | 0.2 | Alarm strip shows only actionable items; journal keeps full history + filters; ack-all works. |
| 2.3 | **Severity tiles / KPI counts** wired to live alarm counts. | `modules/events/`, `aui/` | 0.2 | Counts update as alarms arrive/clear. |
| 2.4 | **Trend workspace + polish**: chart-primary tab with series chips, cursor readout, min/max/avg value grid, and a series inspector (colour, own-pane, limits, annotations); crisp thin grid + limit markings; embedded mini-trend option beside values (`trend.html`). | `modules/graph/`, `graph-qt`, `main_window/` | 1.4, 4.3 | Trend tab matches `trend.html`; overview trend matches `operator-shell`; limits visible; cursor grid live. |
| 2.5 | **Alarm flood affordances**: grouping/counters so floods read as counts, not a scroll. | `modules/events/` | 2.2 | >10/10 min shows a grouped indicator, not raw scroll. |
| 2.6 | **Substation display frame**: wrap the Modus/Vidicon renderer as a workspace tab — `Live` indicator, zoom/fit/export, hotspot breadcrumb, selection → Inspector, bay Measurements + Recent-events strips. Geometry unchanged. | `modules/modus/`, `modules/vidicon/display/`, `main_window/`, `modules/events/` | 1.1–1.5, 4.3 | Matches `substation-display.html`; clicking an element selects it and fills the Inspector; controls open the two-stage confirm. |
| 2.7 | **Equipment-state colouring** applied by the renderer via `--sl-*` tokens (closed/open with shape, energized/de-energized, bad-quality). | `modules/modus/`, `modules/vidicon/display/`, renderer | 0.1, 0.2 | Switching-device state reads by shape **and** colour; energized ≠ alarm-red; matches `design-language.md`. |
| 2.8 | **Table / Watch workspace**: live+historical operator grid — NodeId/formula rows, live values, quality marks, per-row sparklines, timestamps; toolbar (add item/device, remove, move, sort, Live/At-time, add-to-graph, CSV, print); Qt-shaped row context menu; row inspector (`table-watch.html`). | `modules/table/`, `modules/watch/`, `modules/sheet/`, `main_window/` | 0.1–0.4, 4.3 | Matches `table-watch.html`; rows edit/move/delete/sort; menu mirrors the Qt structure with unsupported items disabled + reason; selection fills the Inspector. |

## P3 — Dialog theming (retire native OS widgets)

| # | Item | Touches | Dep | Done when |
|---|---|---|---|---|
| 3.1 | **Login** — 🔶 *theming validated*: under the opt-in theme the real login dialog already recolours cleanly (charcoal-blue surfaces, rounded token fields, accent focus + primary OK) in dark & light — confirmed on the real Qt widget via `client_screenshot_generator --theme=…`. *Remaining (optional, its own slice):* the richer `login.html` treatment (brand lockup + read-only system preview), which needs `.ui` restructuring. | `modules/login/qt/` | — | No native chrome; anonymous + auto-login intact; validated by real-widget screenshot. |
| 3.2 | **Control/Write** — ✅ *done*: dialogs theme cleanly under the opt-in theme (validated on real widgets), **and** the control confirmation is now a present→command review with an irreversibility warning (principle §7) instead of a bare "Switch X to Y?" — control commands still confirm before the operate stage. Fixed a latent dangling-`string_view` in the confirmation prompt while there (message/title now owned by the awaiting coroutine). Unit-tested (`WriteModelTest.ControlCommandConfirmationReviewsPresentAndCommand`). *Optional later:* a bespoke diff dialog with a danger-styled Send button (`control-command.html`) in place of the message box. | `modules/write/` | — | Present→command review shown before control commands; unit-tested; theming validated by real-widget screenshot. |
| 3.3 | **Limits** — ✅ *validated*: warning/alarm limit editor (group boxes + fields) themes cleanly under the opt-in theme on the real widget (`--theme=dark` capture of `limits.png`), no per-dialog code. | `modules/limits/qt/` | — | Themed limit editor; validated by real-widget screenshot. |
| 3.4 | **Remaining modals** (change_password, select_item, time_range, about, create/multi-create). | `modules/*/qt/` | 0.4 | No modal renders native/unstyled. |

## P4 — Explorer & Inspector depth

| # | Item | Touches | Dep | Done when |
|---|---|---|---|---|
| 4.1 | **Explorer status dots** — ✅ *landed (opt-in)*: each object-tree node with a live value shows a quality dot (good/uncertain/bad) composed before its icon, coloured from the severity single source (`QualityColor`) via a new `TreeModel::GetStatusColor` hook fed by `VisibleNodeModel` (`IsBad`/`IsAlerting`). Folders/objects get none. Empty under the legacy theme. Validated on the real object tree (`--theme=dark`); `QualityColor` unit-tested. Inline live values already exist (Value column). | `modules/configuration/objects/`, `aui/qt/tree_model_adapter`, `aui/` | 0.1–0.3 | Each node shows quality dot; validated by real-widget capture. |
| 4.2 | **Explorer filter field.** | `modules/configuration/` | 1.1 | Type-to-filter narrows the tree. |
| 4.3 | **Inspector panel**: big readout, measurements-with-limits, controls (disabled+reason). | new `main_window/qt/inspector`, `modules/write`, `modules/limits`, `modules/node_properties` | 3.2, 3.3 | Selecting a node fills the inspector; controls open themed dialogs; admin-gated actions disabled with reason. |
| 4.4 | **Sidebar extra panes**: Watchlist, Favorites/Portfolio as Explorer siblings. | `modules/watch`, `modules/favorites`, `modules/portfolio` | 1.1 | Panes switch within the sidebar host. |

## P5 — Engineering surfaces & web parity

| # | Item | Touches | Done when |
|---|---|---|---|
| 5.0 | **Configuration / device workbench**: hardware/device tree, tabbed device parameter editor (protocol fields, address map, limits), Revert/Apply, live device-diagnostics inspector (`config-workbench.html`). | `modules/configuration/`, `modules/device_metrics/`, `main_window/` | Matches `config-workbench.html`; device params edit/apply; diagnostics inspector live; unsaved-changes guard. |
| 5.1 | **Users & access rights** admin: users grid + RBAC role/permission editor (inherited-vs-explicit grants), enable/disable, reset password, admin-gated; other node-table admin surfaces (Formats, Simulation, Historical DBs) adopt tokens (`users-admin.html`). | `modules/node_table/`, `modules/change_password/` | Matches `users-admin.html`; editing gated to Administrator; grids themed. |
| 5.2 | **Transmission-rules editor**: rules grid (source → destination IOA, trigger, transform, live status/counts) + rule editor inspector; enable/disable, test-send, Revert/Apply (`transmission-rules.html`). | `modules/transmission/` | Matches `transmission-rules.html`; CRUD + validation; per-rule enable/disable; destination link status shown. |
| 5.3 | **Debugger / protocol trace**: request/response frame trace with direction/type/error coding, filters, pause/clear/save, and a decoded APCI/ASDU + raw-hex inspector; device-metrics themed (`debugger.html`). | `modules/debugger/`, `modules/device_metrics/` | Matches `debugger.html`; live capture with pause/clear/filter/save; frame decode inspector; maps frame → NodeId. |
| 5.4 | **Bulk create / delete wizard**: multi-step flow (target &amp; type → naming/addressing pattern with `{n}` tokens → defaults → review) with a **live preview** grid and per-item conflict resolution (skip/rename/overwrite) (`bulk-create.html`). | `modules/multi_create/`, `Session.addNodes` | Matches `bulk-create.html`; preview updates live from the pattern; conflicts flagged with resolution; create/delete verified by re-browse. |
| 5.5 | **Vocabulary parity check** with `web` component names. | docs | `ActivityBar`/`InspectorPanel`/`StatusStrip`/… names align 1:1 with the web parity matrix (no shared files). |
| 5.6 | **Screenshot regen** for all reshelled views. | `client/tools/screenshot_generator/` | `client/docs/screenshots/` reflects the new UI; manifest tags updated. |

---

## Cross-cutting acceptance (every item)

- Coherent in **light, dark, and high-contrast** themes (principle §5).
- **Keyboard-navigable**; visible focus states.
- **Colour never the only signal** — severity/quality always carry a label or
  shape too.
- Localised via `Translate()` (English in source, Russian in `.ts`).
- A **regression unit test** accompanies behavioural changes (per
  `client/CLAUDE.md`), and the relevant `client/docs/screenshots/` image is
  regenerated in the same change.

## Sizing note

P0–P2 is the bulk of the perceived redesign and the natural first milestone
(the visible operator workbench). P3 is independently shippable and high-impact
per effort (it removes the jarring native dialogs). P4–P5 deepen and complete
coverage. None of it requires touching the data-service or profile-persistence
layers (see [`shell.md`](shell.md) §5).
