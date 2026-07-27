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
│ OperatorTopBar   BrandLockup · Search/Command · alarm KPI tiles    │
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
│ StatusStrip  user·role · connection · latency · endpoint · alarms  │
└────────────────────────────────────────────────────────────────────┘
```

The dock/split/tab machinery the client already has is **kept** — this is a
reshell of the chrome and navigation, not a rewrite of the view layer. Panes
remain dockable and the multi-window profile survives.

## 2. Region-by-region spec and code mapping

### 2.1 Activity bar (new)

A **native left `QToolBar`** on the far left, in two groups separated by a
divider:

1. **Sidebar pane modes** — **Objects, Devices, Files, Nodes**. A mode selects
   which panes occupy the left sidebar and nothing else: Objects → the object
   tree + Portfolio, Devices → the hardware tree, Files → Files + Favorites,
   Nodes → the node tree (admin-only, hidden without the Configure right).
2. **Pages** — one numbered button per page in the profile, plus a **+** that
   creates one. Pages are the coarse navigation: each replaces the whole
   workspace, standing in for the web client's browser tabs. Drag a button to
   reorder; right-click for Rename / Delete / New.

**The rail never opens a workspace tab and never opens a view.** That is the
whole point of the vocabulary — it is a mode switcher plus a page switcher, so
a rail click is always predictable. Surfaces that used to sit here (Alarms,
Trends, Substations, Tables) are reached from the menu bar and the command
palette; the unacknowledged-alarm count lives in the top context bar's severity
tiles and its flood pill.

The two groups carry **independent active markers**, because a pane mode and a
page are both active at once.

> **Native rework (§9).** The rail was specified as a 52 px charcoal strip with
> hand-painted 1.8 px glyphs and an `#activityBar` stylesheet. That is a browser
> idiom and it is being replaced. The prescription is now:
>
> - It is a real `QToolBar` in `Qt::LeftToolBarArea` whose buttons are ordinary
>   `QToolButton`s with `QAction`s — no custom widget painting the rail.
> - **No background repaint.** It takes `QPalette::Window` like any other
>   toolbar; drop the `rail_bg` sheet.
> - **Sizes come from the style**: `QStyle::PM_ToolBarIconSize` /
>   `QStyle::PM_LargeIconSize` and `QFontMetrics`, not `kRailWidth = 52`,
>   `kButtonSize = 44`, `kIconSize = 24`. It must scale with DPI and with the
>   OS font-size accessibility setting.
> - **Icons are real `QIcon`s from the resource set** at multiple sizes (see
>   [`iconography.md`](iconography.md)), not `QPainterPath` glyphs stroked at a
>   fixed pen width — those blur at fractional scaling and ignore
>   high-contrast modes.
> - Active-section marking uses `QAction::setChecked` on an exclusive
>   `QActionGroup`, so the platform draws the affordance it already teaches its
>   users.
> - The badge keeps the **severity token** colour (process semantics are exempt
>   from the platform, §9), but its text colour comes from the palette.
> - **Open question for the rework:** whether a native app should have this rail
>   at all, given it duplicates the menu bar's navigation role (§4). If it stays,
>   it must be user-hideable through the standard toolbar context menu, which a
>   `QToolBar` gives for free and a custom widget does not.

- **Landed (opt-in).** `main_window/activity_bar_qt.{h,cpp}` builds the rail.
  The mode → pane-set mapping is `main_window/pane_modes.{h,cpp}` — Qt-free, so
  the same vocabulary is available to the Wt shell and mirrors the web client's
  `SIDEBAR_MODES`. Switching a mode closes the panes outside it and opens the
  ones inside it (`BaseMainWindow::OpenPaneSync`, close-before-open because the
  view manager tabifies onto the first dock in the area), then fronts the mode's
  first pane.
- **The active marker is derived, never a click artifact.**
  `MainWindow::RefreshPaneModeMarker()` recomputes it from the panes that are
  actually open, on startup, on every page open, on pane close and on view
  activation — so a page switch or a manually closed pane cannot leave the rail
  lying. When the open panes match no mode the marker clears, which is why the
  button group is non-exclusive.
- **The mode is a per-window preference** (`MainWindowDef::pane_mode`), not a
  per-page one: a `Page` already encodes its pane set twice (the `visible` flags
  and the dock blob), and a third representation would need reconciling on every
  save. When a page's stored visibility disagrees with the mode, the mode wins
  and the page is conformed on open — silently and self-healingly.
- **Pages** come from `main_window/pages/page_switcher.{h,cpp}`, the single
  source both the rail and the `MainMenuId::Page` menu read. Order is
  `Page::order`, persisted; drag-and-drop rewrites it, so the menu follows the
  rail for free. New / Rename / Delete run the registered `ID_PAGE_*` commands
  rather than reimplementing them.
- Replaces the role currently played by the `menubar` (File/Edit/View/Window/
  Help) as the *primary* navigation. A conventional menu bar may remain as a
  secondary/keyboard affordance — see §4 open question.

### 2.2 Top context bar (reworked toolbar)

Replaces the grip toolbar. Left: `BrandLockup`. Centre: a **command/search
field** (`Ctrl K`) that searches tags, objects, and commands. Right: **alarm
state only** — the severity KPI tiles (§2.3 of `backlog.md`) and the
alarm-flood pill.

**No identity/connection cluster here.** Who/where context (user·role,
connection, server latency, endpoint·build) belongs to the `StatusStrip`
alone — see §2.7. An earlier revision mirrored those four status panes in the
top bar as a "persistent context cluster"; it rendered them verbatim in both
places, which cost the operator a second place to look without adding a fact
and violated §3's rule against showing one metric in two prominent spots.
Persistent context (principle §8) is satisfied by the status strip, which never
scrolls away. The division of labour is: **top bar = what is wrong now**
(pre-attentive, colour-carrying, changes under alarm), **status strip = where I
am and what I am connected to** (steady, glanceable, rarely changes).

> **Native rework (§9).** The same one-home rule now also settles *which*
> region owns a datum, on native grounds: a desktop application's steady
> context belongs in `QStatusBar`, and that is where every platform's users
> look for it. Additional prescriptions for this bar:
>
> - It is a plain `QToolBar` with no background repaint and no `topbar_bg`.
> - The brand label is dropped. Native applications identify themselves in the
>   window title and the About dialog, not with an in-window lockup — and the
>   current one is a hard-coded, space-padded, untranslated literal.
> - The command/search field keeps its natural `QLineEdit` size hint instead of
>   a fixed 360 px width, so it tracks the OS font size.
> - The alarm-flood pill and severity tiles keep their **severity token**
>   colours (exempt, §9) but must not hard-code their text colour — today the
>   pill bakes in `#ffffff`.
> - The alarm count must appear in **one** place. It is currently rendered
>   three times at once: the status-bar severity panes, these tiles, and the
>   activity-rail badge. Tiles are the pre-attentive "what is wrong now"
>   surface and the rail badge is the navigation affordance; the **status bar's
>   severity panes are the ones to drop**.

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

A **native `QStatusBar`**, and the **single home** for identity and connection
context: user·role, connection, server latency, endpoint·build.

These cells appear **here and nowhere else** — the top bar deliberately does
not mirror them (§2.2). The strip never scrolls away, so it carries the
persistent-context duty of principle §8 on its own.

> **Native rework (§9).**
>
> - **Not charcoal.** It is a `QStatusBar` and takes `QPalette::Window`. A
>   status bar that repaints itself dark under a light system theme is the
>   clearest tell that the window is not native.
> - Cells stay `addPermanentWidget` labels, but drop the fixed pixel widths in
>   favour of `QFontMetrics`-derived hints so they do not clip at larger OS
>   font sizes.
> - **The three alarm/severity panes are removed** (event count, severity
>   summary, highest active severity). They restate what the top-bar tiles
>   already show pre-attentively, and §3's alarm discipline argues against
>   presenting one metric in two prominent places. What remains here is the
>   steady context that changes rarely.
> - The one place a token colour survives is the highest-severity cell *if* it
>   is retained; if it is, it keeps the severity token and gets a text label,
>   never colour alone (§5).

### 2.8 Device log + protocol trace (extends the Watch view)

The device log — the `Watch` view (`WIN` name `Log`, `modules/watch/`), which
subscribes to the device-watch event type and already offers Pause / Clear /
Save — gains a second mode: a **protocol frame trace** for the selected device.
Mocked in [`../ui-mockups/screens/device-protocol-trace.html`](../ui-mockups/screens/device-protocol-trace.html).

**This is device protocol debugging, and it is not the `Debugger` view.** The
`Debugger` (`modules/debugger/`, `--debug` gated) traces client↔server *session
requests* — Browse, Read, Call. The trace specified here decodes the *device*
link: IEC 60870 APCI/ASDU frames, Modbus PDUs, direction and error coding. The
two share a vocabulary and nothing else, and the mockup file was previously
named `debugger.html`, which is why the session debugger's own source comments
came to cite it. They are separate surfaces.

Why it belongs to the device log rather than a Diagnostics page of its own:

- **The scope is already there.** The log view is scoped to a device; the trace
  is the same device seen one layer down. Selecting КП-02 should not mean
  finding a second window and selecting КП-02 again.
- **It is the same task.** An engineer asking "why is this device not
  reporting?" reads the log, and the frame trace is the next question, not a
  different one. §3's alarm discipline argues against scattering one
  investigation across two places.
- **It inherits the log's controls.** Pause, Clear and Save-trace already exist
  on the Watch view and mean the same thing for frames.

Spec:

- A **mode switch within the view** (log ⇄ frame trace), not a separate window.
  The mockup shows them as two tabs over one device sidebar.
- Frame columns: time, direction (RX/TX), frame kind (I/S/U), Type ID, Cause,
  IOA, value, and the send/receive sequence numbers.
- A **decode pane** for the selected frame: raw hex with the decoded APCI/ASDU
  tree beside it, each field annotated with its byte offset, ending at the
  address-space node the object maps to.
- Filters by frame kind and a free-text filter over IOA / type / cause; an
  errors-only filter, since that is the reason the view gets opened.
- **Direction and error coding follow the severity tokens**, not the platform
  (§9): a link-down or t1 timeout row reads as an alarm.
- Capture is **per device and off by default** — a frame trace on a busy link is
  a lot of data, and the mockup's `Capturing · КП-02` status cell exists to make
  an armed capture impossible to forget.

**Landed (first cut).** The mode itself is built: `Frame trace` is a checkable
command on the Watch view (`ID_WATCH_FRAME_TRACE`), and `WatchModel` filters the
same device-watch stream to protocol traffic, with a `Dir` column.

Where the data comes from — the question this section previously left open. The
drivers already tag protocol traffic as they log it: `#` for something received,
`$` for something sent
(`scada-tier-iec104/modules/iec60870/lib/*`, e.g. `"#RX: {} ({} bytes)"` for a
raw frame, `"$TX: Send write confirmation [...]"` for a command). `DeviceLogger`
passes the message into the `DeviceWatchEventType` event untouched, so the
marker survives to the client. `modules/watch/device_log_line.{h,cpp}` reads it.
**No server change was needed for this cut**, which is why it exists at all.

**Frames now arrive as data.** `DeviceFrameEventType` (a subtype of
`DeviceWatchEventType`, so the existing subscription matches it) carries
Direction, RawData, Format, TypeId, Cause, ObjectAddress and the two sequence
numbers. The IEC 60870 driver populates it at the ten sites that observe a PDU;
the trace shows **Type ID, Cause and IOA as sortable columns** rather than text
inside a message.

The marker heuristic remains as a **fallback**, not as the primary path: a
server older than `DeviceFrameEventType` still sends prose with `#`/`$`, and
must keep producing a usable trace. A row without structured data shows its
direction from the marker and leaves the decoded columns blank rather than
guessing values out of the text.

Still missing against the mockup:

- **No decode pane.** `RawData` crosses the wire, but nothing renders the
  APCI/ASDU tree or the hex beside it; the raw frame is still just a log line.
- **N(S)/N(R) are carried but not shown** — the driver does not populate them
  yet (the sequence numbers live in the APCI, below the ASDU the log sites see).
- Eleven driver sites stay plain log lines by design: timer expiries and
  state-machine notes carry the direction markers but describe no PDU, and a
  trace filtered to real traffic is the point of the mode.

## 3. Navigation & interaction rules

- **Keyboard first.** Activity-bar items, tree rows, tabs, tables, and the
  command field are all keyboard-navigable (principle §6; existing Qt focus
  chains preserved). `Ctrl K` opens the command palette.
- **One dominant surface per tab.** Supporting panels stay compact and
  scannable; don't duplicate the same metric in two prominent places. The
  top-bar / status-strip split (§2.2, §2.7) is the worked example: identity and
  connection are stated once, in the strip.
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
