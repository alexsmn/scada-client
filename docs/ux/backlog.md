# SCADA Client — UX Implementation Backlog

> Status: living plan. Turns the reshell design ([`shell.md`](shell.md)) into
> sequenced, module-scoped work. Ordered **operator-first**. Each item names the
> primary code it touches, its dependency, and a "done when" acceptance line.
> This is a design backlog, not a commitment — sequence and sizing are advisory.

## Sequencing at a glance

```
P0 Foundations ──► P1 Shell chrome ──► P2 Operator core ──► P3 Dialog theming
                                   └──► P4 Explorer/Inspector depth
                                                          └──► P5 Engineering & parity
```

Ship P0→P2 before anything else: they deliver the visible operator workbench.
P3 (dialog theming) can run in parallel once P0 lands — it has no dependency on
the shell.

---

## P0 — Foundations (design system in code)

| # | Item | Touches | Done when |
|---|---|---|---|
| 0.1 | **Token layer**: encode the light/dark/high-contrast token sets as a Qt palette + QSS variable sheet, driven by `Profile` theme choice. | `app/qt/`, `profile/`, new `aui/qt/theme_*` | All three themes switch at runtime; no component hard-codes hex; values match [`design-language.md`](design-language.md). |
| 0.2 | **Severity palette single-source**: one enum→token map consumed by tree, journal, alarm strip, inspector, status. | `aui/`, `modules/events/` | Changing a severity token restyles every surface at once. |
| 0.3 | **Mono numerals**: apply `--font-mono` to all value/timestamp/limit cells. | `aui/models/`, table/tree delegates | Values/timestamps render tabular; columns don't jitter on update. |
| 0.4 | **Themed control primitives**: `Field`, `Combo`, `Checkbox`, `Button` (primary/default/danger/disabled+reason). | new `aui/qt/controls` | Primitives available; retire native `QInputDialog`/`QMessageBox` defaults. |

## P1 — Shell chrome

| # | Item | Touches | Dep | Done when |
|---|---|---|---|---|
| 1.1 | **Activity bar** with section commands + Alarms unread badge. | new `main_window/qt/activity_bar`, `core/GlobalCommandRegistry` | 0.1 | Sections switch; badge tracks unacknowledged count. |
| 1.2 | **Top context bar**: brand + command/search field + plant/server/connection/user cluster. | `main_window/`, `core/` | 0.1 | Context cluster always visible; `Ctrl K` opens palette. |
| 1.3 | **Command palette** over `GlobalCommandRegistry` + `SelectionCommandRegistry` + address-space browse. | `core/`, `controller/` | 1.2 | Fuzzy search finds commands, views, and tags; Enter activates. |
| 1.4 | **Editor-style workspace tabs**; drop per-window native title chrome. | `main_window/`, `ViewManager` | 0.1 | Tabs restyled; `OpenedView` labels intact; close/reorder works. |
| 1.5 | **Status strip**: add user·role, unacknowledged count, highest severity, endpoint, build. | `main_window/`, status bar | 0.2 | Strip mirrors top context; alarm summary live. |

## P2 — Operator core (the cockpit)

| # | Item | Touches | Dep | Done when |
|---|---|---|---|---|
| 2.1 | **Overview page** in default `Profile`: KPI severity strip + dominant trend + active-alarm table. | `profile/`, `modules/graph/`, `modules/events/` | 1.1–1.5 | Fresh profile lands on Overview; all three regions live. |
| 2.2 | **Active-alarm surface vs. event journal split** (principle §3): unacknowledged/actionable strip separate from full history; full journal with severity/area/period filters, paging, ack, print/export (`event-journal.html`). | `modules/events/` | 0.2 | Alarm strip shows only actionable items; journal keeps full history + filters; ack-all works. |
| 2.3 | **Severity tiles / KPI counts** wired to live alarm counts. | `modules/events/`, `aui/` | 0.2 | Counts update as alarms arrive/clear. |
| 2.4 | **Trend workspace + polish**: chart-primary tab with series chips, cursor readout, min/max/avg value grid, and a series inspector (colour, own-pane, limits, annotations); crisp thin grid + limit markings; embedded mini-trend option beside values (`trend.html`). | `modules/graph/`, `graph-qt`, `main_window/` | 1.4, 4.3 | Trend tab matches `trend.html`; overview trend matches `operator-shell`; limits visible; cursor grid live. |
| 2.5 | **Alarm flood affordances**: grouping/counters so floods read as counts, not a scroll. | `modules/events/` | 2.2 | >10/10 min shows a grouped indicator, not raw scroll. |
| 2.6 | **Substation display frame**: wrap the Modus/Vidicon renderer as a workspace tab — `Live` indicator, zoom/fit/export, hotspot breadcrumb, selection → Inspector, bay Measurements + Recent-events strips. Geometry unchanged. | `modules/modus/`, `modules/vidicon/display/`, `main_window/`, `modules/events/` | 1.1–1.5, 4.3 | Matches `substation-display.html`; clicking an element selects it and fills the Inspector; controls open the two-stage confirm. |
| 2.7 | **Equipment-state colouring** applied by the renderer via `--sl-*` tokens (closed/open with shape, energized/de-energized, bad-quality). | `modules/modus/`, `modules/vidicon/display/`, renderer | 0.1, 0.2 | Switching-device state reads by shape **and** colour; energized ≠ alarm-red; matches `design-language.md`. |
| 2.8 | **Table / Watch workspace**: live+historical operator grid — NodeId/formula rows, live values, quality marks, per-row sparklines, timestamps; toolbar (add item/device, remove, move, sort, Live/At-time, add-to-graph, CSV, print); Qt-shaped row context menu; row inspector (`table-watch.html`). | `modules/table/`, `modules/watch/`, `modules/sheet/`, `main_window/` | 0.1–0.4, 4.3 | Matches `table-watch.html`; rows edit/move/delete/sort; menu mirrors the Qt structure with unsupported items disabled + reason; selection fills the Inspector. |

## P3 — Dialog theming (retire native OS widgets)

| # | Item | Touches | Dep | Done when |
|---|---|---|---|---|
| 3.1 | **Login** → themed sign-in + read-only system preview. | `modules/login/qt/` | 0.4 | Matches `login.html`; no native chrome; anonymous + auto-login intact. |
| 3.2 | **Control/Write** → themed dialog + **two-stage confirm** with present→command diff and audit meta (principle §7). | `modules/write/qt/` | 0.4 | Matches `control-command.html`; control commands require the confirm step. |
| 3.3 | **Limits** → themed. | `modules/limits/qt/` | 0.4 | Warning/alarm limit editor uses token primitives. |
| 3.4 | **Remaining modals** (change_password, select_item, time_range, about, create/multi-create). | `modules/*/qt/` | 0.4 | No modal renders native/unstyled. |

## P4 — Explorer & Inspector depth

| # | Item | Touches | Dep | Done when |
|---|---|---|---|---|
| 4.1 | **Explorer status dots + inline live values.** | `modules/configuration/`, tree delegate | 0.1–0.3 | Each node shows quality dot + mono value from its subscription. |
| 4.2 | **Explorer filter field.** | `modules/configuration/` | 1.1 | Type-to-filter narrows the tree. |
| 4.3 | **Inspector panel**: big readout, measurements-with-limits, controls (disabled+reason). | new `main_window/qt/inspector`, `modules/write`, `modules/limits`, `modules/node_properties` | 3.2, 3.3 | Selecting a node fills the inspector; controls open themed dialogs; admin-gated actions disabled with reason. |
| 4.4 | **Sidebar extra panes**: Watchlist, Favorites/Portfolio as Explorer siblings. | `modules/watch`, `modules/favorites`, `modules/portfolio` | 1.1 | Panes switch within the sidebar host. |

## P5 — Engineering surfaces & web parity

| # | Item | Touches | Done when |
|---|---|---|---|
| 5.1 | Node-table admin surfaces (Users, Formats, Simulation, Historical DBs) adopt tokens. | `modules/node_table/` | Grids themed; admin gating visible. |
| 5.2 | Transmission-rules editor themed. | `modules/transmission/` | Matches design language. |
| 5.3 | Debugger / device-metrics themed. | `modules/debugger/`, `modules/device_metrics/` | Consistent chrome. |
| 5.4 | **Vocabulary parity check** with `web` component names. | docs | `ActivityBar`/`InspectorPanel`/`StatusStrip`/… names align 1:1 with the web parity matrix (no shared files). |
| 5.5 | **Screenshot regen** for all reshelled views. | `client/tools/screenshot_generator/` | `client/docs/screenshots/` reflects the new UI; manifest tags updated. |

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
