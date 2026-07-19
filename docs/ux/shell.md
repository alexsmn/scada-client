# SCADA Client — Shell Redesign

> Status: living design document. Specifies the reshelled application layout and
> maps every region onto the **existing** client architecture (`main_window/`,
> the command registries, `Profile`, and the `modules/` components). Rationale:
> [`principles.md`](principles.md). Tokens/components:
> [`design-language.md`](design-language.md). Rollout: [`backlog.md`](backlog.md).

## 1. From MDI window to operator workbench

**Today** (`client/docs/screenshots/client-window.png`): a floating toolbar with
a drag-grip, a left "Objects" tree dock, an MDI tab area (Event Journal,
Summary), a bottom event panel, and a status bar. It is functional but reads as
a generic docked desktop app; there is no persistent navigation, no selection
inspector, and dialogs fall back to native OS chrome.

**Proposed** (`operator-shell.html`): the web client's workbench structure —

```
┌────────────────────────────────────────────────────────────────────┐
│ OperatorTopBar   BrandLockup · Search/Command · plant·server·user   │
├──┬───────────────┬───────────────────────────────────┬─────────────┤
│A │ Explorer      │ WorkspaceTabs                      │ Inspector   │
│c │ (object tree, │ ┌───────────────────────────────┐ │ selected    │
│t │  status dots, │ │ KPI severity strip            │ │ item:       │
│i │  live values) │ ├───────────────────────────────┤ │ big readout,│
│v │               │ │ dominant Trend panel          │ │ measurements│
│B │               │ ├───────────────────────────────┤ │ + limits,   │
│a │               │ │ Active alarms / event log     │ │ controls    │
│r │               │ └───────────────────────────────┘ │             │
├──┴───────────────┴───────────────────────────────────┴─────────────┤
│ StatusStrip  user·role · connection · latency · alarm summary       │
└────────────────────────────────────────────────────────────────────┘
```

The dock/split/tab machinery the client already has is **kept** — this is a
reshell of the chrome and navigation, not a rewrite of the view layer. Panes
remain dockable and the multi-window profile survives.

## 2. Region-by-region spec and code mapping

### 2.1 Activity bar (new)

A 52 px charcoal icon rail on the far left, present in every theme. Icons are
the **display hierarchy** (principle §6): **Overview, Alarms, Trends,
Substations, Tables** on top; **Administration, Settings** pinned at the bottom.
The active section carries an accent marker; Alarms carries an unacknowledged
count badge.

- **New surface.** Backed by `GlobalCommandRegistry` (`core/`): each rail item is
  a registered top-level command that activates a section (opens/or focuses its
  default page). The unread badge subscribes to the same alarm-count source the
  `StatusStrip` uses.
- **Landed (opt-in).** `main_window/activity_bar_qt.{h,cpp}` builds the charcoal
  rail; live sections activate their view through `FindWindowInfoByName` →
  `OpenView` (registry-command backing is the follow-up), and the Alarms badge
  reads the new `StatusBarModel::GetAlarmCount()` fed from `EventStatusProvider`.
  Sections with no registered view yet render disabled.
- Replaces the role currently played by the `menubar` (File/Edit/View/Window/
  Help) as the *primary* navigation. A conventional menu bar may remain as a
  secondary/keyboard affordance — see §4 open question.

### 2.2 Top context bar (reworked toolbar)

Replaces the grip toolbar. Left: `BrandLockup`. Centre: a **command/search
field** (`Ctrl K`) that searches tags, objects, and commands. Right: the
**persistent context cluster** — plant/site, server round-trip, connection dot,
current user (principle §8).

- Command palette resolves against `GlobalCommandRegistry` +
  `SelectionCommandRegistry` + address-space browse. This is new UI over
  existing registries. **Landed (opt-in):** `Ctrl K` — or clicking the
  command/search field — opens a modal palette that filters every registered
  command by title (case- and script-insensitive, so a Russian query matches a
  Russian title) and runs the selection through the existing command-handler
  resolution. Implemented in `main_window/command_palette_qt.{h,cpp}` with the
  Qt-free matching in `command_match.{h,cpp}`.

  ![Command palette](../screenshots/command-palette.png)

- Per-view action controls (period, severity filter, print, export,
  acknowledge) move **into the relevant Panel header**, not a global toolbar —
  so the controls sit with the surface they affect.

### 2.3 Explorer sidebar (reworked Objects dock)

The existing configuration/object tree (`modules/configuration/`,
`ConfigurationTreeModel`) gains: a filter field, **status dots** per node
(good/uncertain/bad quality, driven by the same `MonitoredItemService`
subscription that feeds the value column), and right-aligned **live values in
mono**. Selecting a node drives the Inspector.

- Backed by the current tree model + `ViewService`/`AddressSpaceFetcher`. The
  status dot and inline value are presentation additions over data the tree
  already subscribes to.
- The sidebar is the web's extendable-pane host: Explorer first; **Watchlist**
  (`modules/watch/`) and **Favorites/Portfolio** (`modules/favorites`,
  `modules/portfolio`) become additional panes.

### 2.4 Workspace tabs + canvas (reworked MDI)

`MainWindow` → `Page` → `OpenedView` is unchanged underneath; the tab strip is
restyled to editor tabs and the per-window native title chrome is dropped.
`OpenedView::GetWindowTitle()` still supplies the tab label (localised via
`Translate()`).

- The **Overview** canvas is a new Level-1 composition: `SeverityTile` KPI strip
  + a dominant `Graph` (`modules/graph/`) + the active-alarm/event table
  (`modules/events/`). It is assembled as a `Page` in the default `Profile`, so
  no new layout engine is required — it is a pre-populated page.
- Other tabs (trend, journal, table, sheet) are the existing views rendered
  inside the new tab chrome.

### 2.5 Inspector (new)

A ~308 px right panel driven by the current selection: a large mono read-out
with quality pill, a **Measurements** section that shows the live value
**beside its warning/alarm limits** (principle §2), source protocol, and a
**Controls** section (`Control…`, `Manual input…`, `Limits…`, `Unlock`).

- Composed from existing components surfaced today only as modal dialogs:
  `modules/write/` (control/manual), `modules/limits/`, `modules/node_properties/`.
  The Inspector is a **persistent host** for these; the actions still open the
  themed dialogs for confirmation (principle §7).
- Controls the user/capability can't perform are **disabled with a reason**
  (`WIN_REQUIRES_ADMIN` on `WindowInfo` is the existing front-line check).

### 2.6 Substation / schematic display (reframed Modus/Vidicon)

The single-line mimic (UC-11, FR-19/20) is the primary **Level-2** operator
surface. Today it is a Modus 6.30 ActiveX control or a Vidicon display embedded
raw (`modules/modus/`, `modules/vidicon/display/`), with native chrome and no
shared visual language. The reshell wraps it as a first-class workspace tab —
see `substation-display.html`.

**Scope boundary (important):** the **diagram geometry is authored content**
— it comes from the customer's Modus/Vidicon display file (edited in the
Designer, rendered by Modus ActiveX on desktop and `web/packages/display-renderer`
on web). The design system does **not** redraw it. What the shell *does* own:

- **Display frame** — the tab, a `Live`/paused indicator, zoom/fit and export
  controls, and a **hotspot-navigation breadcrumb** (site → voltage level →
  bay). Replaces per-control native chrome.
- **Equipment-state colouring** applied by the renderer via the
  `--sl-*` tokens (see [`design-language.md`](design-language.md)): closed vs
  open switching devices (shape **and** colour), energized vs de-energized
  conductors (calm, not alarm-red per principle §1), and bad-quality marking.
- **Selection → Inspector → control.** Clicking an element selects it (accent
  halo), fills the Inspector (§2.5) with its state, measurements, and the
  two-stage `Open…`/`Close…` control (principle §7); controls the user can't
  perform are disabled with a reason.
- **Context strips beneath the diagram** — compact bay Measurements and
  bay-scoped Recent events (`modules/events/`), per the web Substation guidance.
- **Click-to-navigate hotspots** (FR-19) drive the breadcrumb and open faceplates
  / child displays.

This is presentation and interaction plumbing **around** the existing renderer;
it does not change how a `.vds`/Modus drawing is parsed or drawn. On platforms
where the ActiveX control is unavailable (non-Windows, Wt/web), the same frame
hosts the cross-platform display renderer instead — the frame and controls are
identical, only the drawing backend differs.

### 2.7 Status strip (reworked status bar)

Charcoal bottom strip mirroring the top context: user·role, connection, server
latency, **unacknowledged count**, **highest active severity**, endpoint, build.
Extends the current status bar (`Events / Severity / Connected / Server ms`) with
the alarm summary and identity cells.

## 3. Navigation & interaction rules

- **Keyboard first.** Activity-bar items, tree rows, tabs, tables, and the
  command field are all keyboard-navigable (principle §6; existing Qt focus
  chains preserved). `Ctrl K` opens the command palette.
- **One dominant surface per tab.** Supporting panels stay compact and
  scannable; don't duplicate the same metric in two prominent places.
- **Selection is global.** Explorer selection → Inspector → per-view "add to
  active table/graph" (already the web policy; mirror on desktop via
  `SelectionCommandRegistry`).
- **Alarms auto-surface** but never steal control focus: the `EventDispatcher`
  auto-show/flash/sound behaviour drives the Alarms badge and KPI strip.

## 4. Open questions (decide during rollout)

1. **Menu bar retention.** Full reshell implies the Activity bar + command
   palette replace the File/Edit/View/Window/Help menu as primary navigation.
   Desktop users expect a menu bar for discoverability and OS integration
   (macOS global menu). *Recommendation:* keep a slim menu bar for
   completeness/keyboard/OS integration, but treat the Activity bar + command
   palette as the primary path. Confirm with stakeholders.
2. **Overview as default page vs. restore last profile.** Should first launch
   land on the Level-1 Overview, or restore the operator's last `Profile` page
   set? *Recommendation:* land on Overview for a fresh profile; restore
   otherwise.
3. **Inspector docking.** Fixed right panel vs. dockable/closable like other
   panes. *Recommendation:* dockable but shown by default; persist in `Profile`.
4. **Wt/web parity of the shell.** The web already has this shell; ensure new
   desktop chrome names stay aligned with `web` component names
   (`ActivityBar`, `InspectorPanel`, `StatusStrip`, …) so the parity matrix
   stays one-to-one. No cross-repo code dependency — shared *vocabulary*, not
   shared *files*. The matrix lives in
   [`vocabulary-parity.md`](vocabulary-parity.md) (backlog 5.5): desktop ⇄ web
   component names, drift, and reconciliation action items.

## 5. What explicitly does not change

- The `Profile` persistence model, `Page`/`OpenedView` lifecycle, `ViewManager`
  docking, the data-service backends (Scada/OPC UA/Vidicon), and the coroutine
  I/O boundaries. This redesign is **chrome, navigation, selection, and dialog
  theming** — it rides on the existing view and service layers.
