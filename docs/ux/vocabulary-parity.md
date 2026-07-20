# Component vocabulary parity (desktop ⇄ web)

Backlog item **5.5**. The reshelled desktop client and the `web` client are two
implementations of the same operator/engineering workbench. To keep them
learnable as one product, their shell regions and feature surfaces should share
a **vocabulary** — the same concept goes by matching (or deliberately mapped)
names on both sides.

This matrix is that mapping. It is a **client-owned copy** of the shared
vocabulary: it names web components for reference but creates **no cross-repo
file dependency**. Per the repo rule (`scada/CLAUDE.md` → Restrictions and
`docs/ux/shell.md` §4.4), the desktop tree never references `web/` files; when a
shared asset is needed it is copied here or moved to a neutral location. Shared
*vocabulary*, not shared *files*.

## How to read it

- **Concept / surface** — the workbench region or feature, named neutrally.
- **Web component** — the canonical name in the web design system
  (`web/ds-bundle/components/<area>/<Name>`) or the workspace-shell feature
  (`web/apps/app/src/features/…`). These are the source of truth for the shared
  noun.
- **Desktop component** — the reshell class / module in this repo.
- **Status**:
  - **Aligned** — same concept, matching or accepted-equivalent name.
  - **Drift** — same concept, different name; reconcile (see Action items).
  - **Desktop-only** / **Web-only** — exists on one side only (not necessarily
    wrong; desktop and web shells differ in places).

Names were gathered from `web/ds-bundle/components/**` and the desktop reshell
classes; verify against those when updating (see Maintenance).

## Shell regions

| Concept / surface | Web component | Desktop component | Status |
|---|---|---|---|
| Workspace shell / layout | `layout/WorkspaceLayout` | `MainWindow` (`main_window/`) | Aligned (concept) |
| Activity rail | `workspace-shell` `RailButton` (in `WorkspaceSidebar`) | `ActivityBar` (`main_window/activity_bar_qt`) | Drift — web *Rail*, desktop *ActivityBar* |
| Explorer / primary sidebar | `workspace-shell` `WorkspaceSidebar` + `address-space/AddressSpaceTree` | `Explorer` = `ConfigurationTreeView` (`modules/configuration`) | Drift — web *AddressSpaceTree*, desktop *ConfigurationTreeView* |
| Workspace tabs | `workspace-shell/qt-flexlayout-adapter`, `QtWorkspaceWindow` | `WorkspaceTabs` = `ViewManager` / `OpenedView` tabs | Aligned (concept) |
| Inspector / secondary pane | `workspace-shell/Panel` + `InsightList` + `EmptyPanel` | `InspectorPanel` (`modules/inspector`) | Drift — web generic *Panel/InsightList*, desktop *InspectorPanel* |
| Top-bar chips | `workspace-shell/TopBarChips`, `connection-status/ConnectionBadge` | `ContextBar` (`main_window` context bar) | Drift |
| Status strip | (web folds status into `TopBarChips` / `ConnectionBadge`) | `StatusStrip` = `StatusBarController` (`main_window/status_bar`) | Desktop-distinct |
| Command palette | `commands/CommandPalette` | `MainWindow::ShowCommandPalette` | Aligned |
| Action toolbar | `commands/WorkspaceActionToolbar`, `CommandOverflow` | context-bar actions (no distinct class yet) | Web-fuller |
| Brand lockup | `workspace-shell/BrandLockup` | brand cell in `ActivityBar` / context bar | Aligned (concept) |
| Theme switch | `theme/ThemeProvider`, `theme/ThemeSwitcher` | `scada::aui::ApplyTheme` (`aui/qt/theme_qt`) | Aligned (concept) |

## Engineering & operator surfaces

| Concept / surface | Web component | Desktop component | Status |
|---|---|---|---|
| Config / device parameters | `node-table/NodeTable` | `DeviceParameterForm` (`modules/parameter_form`) | Drift |
| Device diagnostics / metrics | `device-metrics/DeviceMetrics` | `DeviceDiagnosticsPanel` (`modules/device_diagnostics`) | Drift |
| Users & access (RBAC) | `change-password/ChangePasswordForm` (+ users in `node-table`) | `UserAccessPanel` (`modules/user_access`) | Drift — no web RBAC inspector counterpart |
| Transmission rules | `transmission/TransmissionRulesEditor` | `TransmissionRuleInspector` (`modules/transmission_rules`) | Drift — *Editor* vs *Inspector* |
| Debugger / protocol trace | `debugger/ProtocolDebugger` | `Debugger` (`modules/debugger`) | Drift — *ProtocolDebugger* vs *Debugger* |
| Bulk create | `bulk-create/BulkCreateWizard` | `BulkCreatePreviewPanel` (`modules/bulk_create`) | Drift — web full *Wizard*, desktop preview panel |
| Trends / series inspector | `trends/TrendsView`, `SeriesChips`, `SeriesStatsTable`, `TrendInsights`, `TrendToolbar`, `Graph` | `SeriesInspector` + graph (`modules/graph`) | Partial |
| Substation display | `substation/SubstationView`, `DisplayRendererCanvas`, `EquipmentDetailsCard`, `EquipmentStateCards`, `LiveMeasurementsTable`, `SubstationKpiStrip` | `DisplayFrame` (`modules/display_frame`) + VDS renderer | Partial |
| Events journal | `events/EventJournal`, `EventPane`, `EventsPanel` | event journal / `EventFilterBar` (`modules/events`) | Aligned |
| Table / sheet | `table/QtTableView`, `table/Sheet` | node table / sheet + `TableToolbar` (`modules/table`, `modules/sheet`) | Aligned |
| Watch list | `watch/WatchList` | `WatchModel` view (`modules/watch`) | Aligned |
| Favorites | `favorites/FavoritesPanel` | favorites (`modules/favorites`) | Aligned |
| Portfolio | `portfolio/PortfolioPanel` | portfolio (`modules/portfolio`) | Aligned |
| Files | `files/FileBrowser` | `FileSystemView` (`modules/filesystem`) | Drift — *FileBrowser* vs *FileSystemView* |
| Write value | `write/WriteValueDialog` | write dialog (`modules/write`) | Aligned |
| Login | `login/LoginForm`, `LoginHeroPreview` | login (`modules/login`) | Aligned |
| Export / snapshot | `export/SnapshotPanel` | export (`modules/export`) | Aligned (concept) |
| Device watch | `device-watch/DeviceWatch` | device-watch view | Aligned |
| Alarm KPIs | `alarms/AlarmSeverityTiles`, `alarms/AlarmWatchdog` | severity KPI tiles in the context bar | Web-fuller |
| Time range | `time-range/TimeRangePicker` | graph time-range control | Partial |
| Locale switch | `i18n/LocaleSwitcher` | Qt translator / language menu | Aligned (concept) |

## Action items (naming drift to reconcile)

The reshell is the moving side, so drift is resolved by renaming the **desktop**
term toward the web noun (or, where the desktop term is the agreed product
noun — `ActivityBar`, `StatusStrip`, `Inspector` from `shell.md` — recording
that decision here and leaving web as the outlier). None of these are code
dependencies; they are naming conventions for new/renamed classes.

1. **`TransmissionRuleInspector` → align with `TransmissionRulesEditor`.** The
   desktop panel is currently rule-*inspector* scoped; as the grid/editor
   surface lands (backlog 5.2 remainder), name the composite the *editor* to
   match web.
2. **`BulkCreatePreviewPanel` → part of a `BulkCreateWizard`.** Web names the
   whole flow a *wizard*; the desktop preview panel is one step. Keep the panel
   name for the step, introduce a `BulkCreateWizard` shell when the multi-step
   navigation lands (backlog 5.4 remainder).
3. **`Debugger` vs `ProtocolDebugger`.** Web is explicit that it traces
   protocol frames; the desktop `Debugger` traces client↔server session
   requests (see 5.3 scope note). Keep the desktop name but document the scope
   difference so the shared noun isn't over-promised.
4. **`FileSystemView` vs `FileBrowser`, `ConfigurationTreeView` vs
   `AddressSpaceTree`.** Pre-reshell desktop names; rename opportunistically
   when these surfaces are next reshelled, not as a churn-only change.
5. **Shell-region nouns** (`ActivityBar`, `Explorer`, `Inspector`,
   `StatusStrip`, `ContextBar`) are the **agreed desktop product vocabulary**
   from `shell.md`; web composes the same regions from finer-grained components
   (`RailButton`, `WorkspaceSidebar`, `Panel`, `TopBarChips`,
   `ConnectionBadge`). This is an accepted granularity difference, not drift to
   fix — recorded so the two shells read as one product.

## Maintenance

- Update this matrix in the same change that **renames a reshell class**, **adds
  a new reshelled surface**, or **learns a web component name changed**.
- Re-derive the web column from `web/ds-bundle/components/**` and the desktop
  column from the reshell classes; do not import either as a build dependency.
- Cross-referenced from [`shell.md`](shell.md) §4.4 and the
  [UX README](README.md).
