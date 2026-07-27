# SCADA Client — UX Design

This directory holds the UX design system for the Telecontrol SCADA desktop
client (Qt). It defines an **operator workbench that looks and behaves like a
native desktop application** on each host OS, grounded in established
industrial HMI standards.

## Direction (agreed 2026-07-26 — supersedes the earlier "match the browser" direction)

- **Native look and feel is the goal.** The client is a desktop application and
  must read as one: the **platform Qt style** (`windows11`/`windowsvista` on
  Windows, `macos` on macOS, the `QT_QPA_PLATFORMTHEME` style on Linux), native
  metrics, native focus and selection behaviour, native menus and dialogs. We
  do **not** force Fusion, and we do not reproduce a browser workbench in Qt.
- **Palette-first, stylesheet-last.** Colour comes from `QPalette` roles that
  the platform style already honours. A global QSS sheet that repaints native
  controls is a regression, not a feature — every removed `setStyleSheet` is
  progress.
- **Follow the OS light/dark preference by default**, with an explicit
  dark / light / high-contrast override for control rooms that need one. Dark
  remains the recommended control-room setting, but it is no longer forced.
- **Metrics come from the platform**, not from fixed pixel constants:
  `QStyle::PixelMetric`, `QFontMetrics`, `QApplication::font()`. Hard-coded
  `font-size:11px`, fixed rail widths and hand-tuned radii are defects — they
  break DPI scaling, OS font-size accessibility settings, and platform
  conventions.
- **Process semantics stay ours, not the platform's.** Alarm severity, data
  quality (good/uncertain/bad), and single-line equipment state are *functional*
  colours mandated by ISA-101 / ISA-18.2 / EEMUA 191. They must **not** follow
  the OS accent colour or invert with the system theme. This is the one
  deliberate exception to palette-first, and it is a safety requirement.
- **Structural layout is a desktop idiom**: a real menu bar, real `QToolBar`s,
  real `QDockWidget`s, a real `QStatusBar`, real tabs. Each datum has exactly
  one home — persistent identity/connection context lives in the status strip
  and is not restated in the top bar.
- **Operator-first**: live monitoring, alarms/acknowledge, trends, and
  control/write commands are designed first.

> **Capability parity, not visual parity.** This client is expected to be able
> to do everything the browser client can, and vice versa. It is *not* expected
> to look like it. Divergence in chrome, colour and interaction idiom is the
> intended outcome of this direction, not drift to reconcile.

## Documents

| Doc | What it covers |
|---|---|
| [`principles.md`](principles.md) | Why the UI behaves as it does — HMI/SCADA standards (High-Performance HMI, ISA-101, ISA-18.2/EEMUA 191, Endsley situational awareness, colour rules) with citations, each traced to a mockup. |
| [`design-language.md`](design-language.md) | The semantic vocabulary: which palette role or process-semantic token each surface uses, the themes, and the component primitives. |
| [`iconography.md`](iconography.md) | The icon set (Lucide, ISC), geometry and size tokens, the `qtsvg` → `.qrc` → `QIcon` pipeline, the full command→glyph map, and how to add or change an icon. |
| [`shell.md`](shell.md) | The reshelled layout, region by region, mapped onto the **existing** client code; navigation rules; open questions. |
| [`dialogs.md`](dialogs.md) | Dialog conventions: the title bar *is* the title, `QDialogButtonBox` for platform button order, naming the action, destructive actions, keyboard and modality. |
| [`vocabulary-parity.md`](vocabulary-parity.md) | Desktop ⇄ web component-name parity matrix (backlog 5.5): shared vocabulary, drift, and reconciliation action items. No cross-repo file dependency. |
| [`backlog.md`](backlog.md) | Operator-first, module-scoped implementation plan (P0→P5) with acceptance lines. |

## Mockups

> **Restyled 2026-07-26 to the native direction.** Their chrome now approximates
> a neutral platform palette: OS-like window/base greys instead of the old
> blue-black token set, no charcoal rail or status strip, the system accent for
> selection, a system font stack, no brand lockup, and dialogs drawn under a real
> title bar rather than repeating it in their content. Process semantics —
> severity, quality, single-line equipment state — are unchanged, because those
> are fixed safety colours ([`principles.md`](principles.md) §9).
>
> **They remain approximations, not a visual target.** No HTML can be faithful
> here: "native" means the appearance is the host platform's, and it differs
> between macOS and Windows. Read the mockups for *what goes where and which
> data appears*; take the *appearance* from the platform. Validate anything
> implemented against real Qt widgets via the headless
> `client_screenshot_generator`, never against the HTML.

Rendered, theme-toggleable HTML in
[`../ui-mockups/screens/`](../ui-mockups/screens/):

| File | Shows |
|---|---|
| `operator-shell.html` | The full operator workbench (dark default + light toggle). |
| `substation-display.html` | Single-line mimic display in the shell — equipment state, live values, click-to-control. |
| `trend.html` | Chart-primary trend workspace — series chips, cursor readout, min/max/avg grid, series inspector. |
| `event-journal.html` | Filterable event/alarm journal — severity filters, acknowledge, area filter, event inspector. |
| `table-watch.html` | Live + historical operator grid — formulas/NodeIds, quality, embedded sparklines, Qt-shaped context menu, row inspector. |
| `config-workbench.html` | Engineering surface — device/hardware tree, tabbed device parameter editor (IEC 60870 fields, address map), live device-diagnostics inspector. |
| `users-admin.html` | Admin surface — users grid + RBAC role/permission editor with inherited-vs-explicit grants, admin-gated. |
| `device-protocol-trace.html` | **Device log — protocol trace.** IEC 60870 frame trace with decoded APCI/ASDU tree, raw hex and direction/error coding, shown as a mode of the device log view. This is *device* protocol debugging — not the `Debugger` view, which traces client↔server session requests and has its own capture (`debugger.png`). |
| `transmission-rules.html` | Re-transmission rules grid (source → destination IOA, trigger, transform, status) + rule editor. |
| `bulk-create.html` | Bulk create-many wizard — stepper, naming/addressing pattern, live preview with conflict resolution. |
| `login.html` | Themed sign-in with read-only system preview (replaces the native OS dialog). |
| `control-command.html` | Two-stage control/write confirm dialog. |
| `main-window-dark.html` | Earlier exploration (kept for reference). |

To view them locally:

```bash
cd client/docs/ui-mockups/screens && python3 -m http.server 8731
# open http://localhost:8731/operator-shell.html
```

## Relationship to the rest of the docs

- Current architecture: [`../design.md`](../design.md) and
  [`../requirements.md`](../requirements.md).
- Current UI screenshots (the *before*): [`../screenshots/`](../screenshots/).
- [`vocabulary-parity.md`](vocabulary-parity.md) keeps the **names** of shell
  regions and surfaces aligned across the product's two front ends. Shared
  nouns are still worth having — they make the two clients learnable as one
  product and keep feature discussions unambiguous. Shared *appearance* is not,
  and is no longer a goal.

## Maintenance

Treat these as living documents. When a token, component, or shell region
changes: update the relevant doc **and** the affected mockup in the same change,
and regenerate the touched `client/docs/screenshots/` image once the code lands
— the same discipline the screenshot generator already follows.
