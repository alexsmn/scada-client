# SCADA Client — Design Language (tokens & components)

> Status: living design document. The client's shared visual vocabulary:
> colour, type, spacing, and the component primitives every screen is built
> from. Rationale is in [`principles.md`](principles.md); layout is in
> [`shell.md`](shell.md).

> **Direction change (2026-07-26).** These tokens are no longer a palette to
> paint the application with. Per [`principles.md`](principles.md) §9 the client
> takes its **chrome colour from the platform** — the native Qt style and the
> `QPalette` it honours. The tables below are retained for two narrower jobs:
>
> 1. **Process semantics** (severity, quality, single-line equipment state) —
>    still authoritative, still fixed, deliberately **not** platform-derived.
>    These are safety signals; see §9 and the exception note there.
> 2. **The explicit dark / light / high-contrast override**, for control rooms
>    that standardise on one appearance instead of following the OS.
>
> **Surface and text tokens are being retired** in favour of `QPalette` roles.
> Treat the `--bg` / `--surface` / `--fg` / `--border` rows as a *mapping table*
> — "which palette role does this surface use" — not as hex to hard-code. New
> code must not introduce a `setStyleSheet` that bakes one of them in.

**Under the System theme these tokens *are* the OS colours.** `GetThemeTokens(Theme::kSystem)`
derives the chrome half of the table from the live `QPalette` — so the ~23
surfaces that still style themselves from tokens follow the desktop
automatically, without waiting for the per-widget conversion (P6.4). The
mapping below is therefore both "what to use in new code" and "what the System
theme actually does".

One platform caveat is handled in code: `QPalette::AlternateBase` is meant to be
a barely-there stripe beside `Base`, but the macOS style reports a mid grey
(`#8e8e8e`) for it. Taken literally that lights up every table header on a dark
window, so an `AlternateBase` too far from `Base` is ignored in favour of a
shade derived from `Base`.

**Token → palette role mapping** (use the right-hand column in new code):

| Token | Use instead |
|---|---|
| `--bg` | `QPalette::Window` |
| `--bg-elevated`, `--topbar-bg` | `QPalette::Window` (let the style differentiate toolbars/docks) |
| `--surface` | `QPalette::Base` |
| trend plot canvas | `QPalette::Base` — a chart canvas is a data surface like a table or tree view, **not** a tinted signal surface |
| `--surface-muted` | `QPalette::AlternateBase` / `QPalette::Button` |
| `--rail-bg` | `QPalette::Window` — **the charcoal rail is retired**; a native toolbar does not repaint its background |
| `--fg` | `QPalette::WindowText` / `QPalette::Text` |
| `--fg-muted`, `--fg-subtle` | `QPalette::PlaceholderText`, or `WindowText` at reduced opacity |
| `--border`, `--border-strong` | `QPalette::Mid` / `QPalette::Dark`, or let the style draw the frame |
| `--accent`, `--accent-soft` | `QPalette::Highlight` / `QPalette::HighlightedText` — **the OS accent colour**, not ours |
| `--good` / `--uncertain` / `--bad`, severity ramp, `--sl-*` | **keep the token** — process semantics, exempt from the platform |

> **Implementation.** The token tables and the `QPalette` builder live in
> [`aui/qt/theme_qt.{h,cpp}`](../../aui/qt/theme_qt.h)
> (`scada::aui::ThemeTokens` / `scada::aui::ApplyTheme`).
> `BuildThemePalette()` is the part that survives and grows — it carries the
> entire appearance on its own. `BuildThemeStyleSheet()` has been reduced to a
> single rule (`QPushButton[role="danger"]`, the one thing with no palette
> role); its removed blocks are listed in the source so they do not creep back.
> Every `setStyleSheet` call removed from a widget is progress; every one added
> needs a reason that `QPalette` and `QStyle::PixelMetric` could not serve.

## 1. Themes

**System is the default.** The client follows the OS light/dark preference
(`QStyleHints::colorScheme()`) unless the operator picks an explicit theme:

- **System** — *default*. Track the host OS appearance and switch live when the
  user changes it. On a machine with no preference this resolves to the
  platform's own default.
- **Dark** — explicit override. Recommended for control rooms (low glare at
  night), but no longer forced on every install.
- **Light** — explicit override.
- **High contrast** — accessibility / bright-ambient fallback. Where the OS
  reports its own high-contrast mode, honour that instead.

The operator can switch at runtime; the choice persists in the `Profile`.
Switching must not require a restart, which means every themed surface has to
react to `QEvent::ApplicationPaletteChange` — today only `Tree`, `Table` and
the trend chart do (`aui/qt/tree.cpp`, `aui/qt/table.cpp`,
`third_party/graph_qt/graph.cpp` + `modules/graph/metrix_graph.cpp`). Every
other surface still needs converting.

Two things the chart conversion showed, both of which generalise:

- **Children that inherit the palette are repainted for free.** `GraphPane`,
  `GraphPlot` and `GraphAxis` all resolve their colours from `Graph` rather than
  from their own palettes, yet they still repaint correctly, because Qt
  propagates a palette change down to every child that has not set one of its
  own. Give a child its own palette and that stops — which is what
  `GraphRenderingTest.PaletteChangeRepaintsChildren` guards.
- **A colour written with `setPalette()` does not follow the OS afterwards** —
  it is a resolved entry, so the widget stops inheriting that role. Anything
  that paints a token onto a role (as the explicit Dark/Light themes do) has to
  re-apply it on `ApplicationPaletteChange`, and must first check whether the
  operator pinned a colour of their own.

## 2. Colour tokens

Semantic tokens only — **components never hard-code hex**. Grouped by role.

### Surfaces & text

**The chrome is neutral grey in every theme, deliberately.** It used to be
blue-tinted throughout — charcoals like `#07111b` … `#152635` in dark, faint
blue whites like `#f5f8fb` in light. Hue in the chrome competes with the only
thing allowed to carry hue, process semantics, and a tinted window is the tell
that an application is painting its own idea of light/dark rather than sitting
inside the desktop's. Every surface and text token below is fully desaturated,
and `ThemeQtTest.ChromeSurfacesAndTextAreNeutral` keeps it that way. The values
match the mockups in [`../ui-mockups/screens/`](../ui-mockups/screens/).

| Token | Dark | Light | Role |
|---|---|---|---|
| `--bg` | `#1e1e1e` | `#ececec` | app background |
| `--bg-elevated` | `#1e1e1e` | `#ececec` | sidebars, tab bar, dialog footers |
| `--surface` | `#171717` | `#ffffff` | panels, cards, dialogs |
| `--surface-muted` | `#292929` | `#f2f2f2` | table headers, inset fields |
| `--rail-bg` | `#1e1e1e` | `#ececec` | Activity bar & status strip — now just `--bg`; see Retired below |
| `--topbar-bg` | `#1e1e1e` | `#ececec` | top context bar |
| `--fg` | `#e6e6e6` | `#1d1d1f` | primary text |
| `--fg-muted` | `#b4b4b4` | `#4a4a4d` | secondary text |
| `--fg-subtle` | `#8c8c8c` | `#6e6e73` | labels, captions |
| `--border` | `rgba(255,255,255,.10)` | `rgba(0,0,0,.12)` | hairlines |
| `--border-strong` | `rgba(255,255,255,.18)` | `rgba(0,0,0,.22)` | field/control borders |

> **Retired.** The charcoal-rail-on-light-workspace signature is dropped. A
> toolbar and a status bar that repaint themselves dark while the rest of the
> window follows the OS is the most conspicuously non-native thing the client
> does, and it is exactly what an operator reads as "this is a web page in a
> window". Under §9 the activity bar and status strip take
> `QPalette::Window` like any other native chrome, in every theme. `--rail-bg`
> and its companion `--fg-on-dark` are now aliases of `--bg` and `--fg`; they
> survive only until backlog P6.3 deletes them, and
> `ThemeQtTest.RailIsOrdinaryChromeInEveryTheme` stops a distinct value from
> creeping back.

> **Accent is not covered by this.** `--accent` stays `#77b4f3` / `#0f6bff`
> rather than the mockups' `#0a84ff` / `#0071e3`. Under the System theme the
> accent comes from `QPalette::Highlight` — the operator's own desktop accent —
> so the value in these two explicit tables is only a stand-in for a machine
> with no preference, and matching one platform's blue exactly would be a false
> precision.

### Accent & quality

| Token | Dark | Light | Role |
|---|---|---|---|
| `--accent` | `#77b4f3` | `#0f6bff` | active/navigation, primary action, focus |
| `--accent-soft` | `rgba(119,180,243,.15)` | `rgba(15,107,255,.1)` | selection fill, active tint |
| `--good` | `#44c091` | `#14825f` | healthy / in-service / good quality |
| `--uncertain` | `#e6b24b` | `#b67a17` | uncertain/stale quality |
| `--bad` | `#f07168` | `#c53d35` | bad quality / comms loss |

**Blue means active/navigation, green means healthy, amber/red mean
abnormal** — colour stays functional (principle §1, §5).

### Severity ramp (alarms)

One ramp, used identically on every surface. Each has a `-soft` (row/tile fill)
and `-fg` (text-on-solid) companion.

| Token | Dark | Light | Priority |
|---|---|---|---|
| `--severity-critical` | `#e85a52` | `#8f241f` | Critical |
| `--severity-high` | `#f07168` | `#b7312b` | High |
| `--severity-medium` | `#e6b24b` | `#c18a24` | Medium |
| `--severity-low` | `#77b4f3` | `#235f98` | Info / low |

Note the light ramp is **deliberately dark and desaturated** (deep reds, not
fire-engine red): alarm colour must signal *abnormal* without turning a busy
journal into a wall of saturated red (principle §1). High-contrast theme swaps
these for maximum-separation values (`#ff6b6b`, `#ff9f43`, `#ffff00`, `#00d4ff`).

### Single-line diagram semantics (schematic displays)

Mimic/single-line displays (Modus/Vidicon schematics) apply an additional
**equipment-state** token set so the *renderer* colours breakers,
disconnectors, busbars, and conductors consistently — independent of the
authored drawing. Kept calm per High-Performance HMI: an energized, all-closed
bay is not "all green everywhere".

| Token | Dark | Light | Meaning |
|---|---|---|---|
| `--sl-live` | `#e6b24b` | `#b67a17` | energized primary conductor / live busbar (restrained amber, **not** alarm-red) |
| `--sl-energized` | `#8fa3b4` | `#6b7b8d` | de-energized / idle conductor (neutral) |
| `--sl-closed` | `#44c091` | `#14825f` | switching device **closed / in service** |
| `--sl-open` | `#8fa3b4` | `#8a99a8` | switching device **open** (neutral — an open breaker is not an alarm) |
| selection | `--accent` dashed halo | same | operator-selected element → drives the Inspector |

Rules: breaker/disconnector state carries **shape as well as colour** (filled
square = closed, hollow = open; principle §5); bad-quality telemetry marks the
symbol and its label, it does not silently freeze; the diagram geometry itself
is authored content and is never restyled — see [`shell.md`](shell.md) §2.6.

## 3. Typography

| Token | Value | Use |
|---|---|---|
| `--font-sans` | Inter → Segoe UI Variable → system-ui | all UI chrome and labels |
| `--font-mono` | Cascadia Mono → Consolas → ui-monospace | **every value, NodeId, timestamp, and measurement** |

- **Tabular / monospace numerals everywhere a value can change or align** —
  live values, timestamps, KPIs, limits. This keeps columns steady as digits
  update (principle §2, perception).
- Restrained type scale. Reserve large type for login branding and true page
  titles only; panel headings are compact and sentence-short.
- Letter spacing `0`; no viewport-scaled type.

## 4. Spacing, radius, density

- Spacing steps: **8 px** tight groups, **12 px** panel padding, **16 px** major
  gutters. Tables are compact (row height ~21–24 px) for repeated operator use.
- Radius: `--radius-sm 4px`, `--radius-md 6px`, `--radius-lg 8px`. Nothing more
  rounded — this is a workbench, not a marketing page.
- **No nested cards.** Group with a panel header, a separator, or a table
  section — never a card inside a card. Prefer thin separators and flat panels
  over shadow-stacked surfaces.

## 5. Component primitives

The shared kit every screen composes from. Qt implementations live in the
existing modules (see [`shell.md`](shell.md) for the mapping); names mirror the
web components so parity discussions use one vocabulary.

| Primitive | What it is | Notes |
|---|---|---|
| **BrandLockup** | `TC` mark + "Telecontrol SCADA" | top-left of top bar & login |
| **ActivityBar** | charcoal icon rail, active marker, unread count badge | maps to display Level 1–4 sections |
| **SidebarPanel / Explorer** | header + filter + tree/list body | object tree with status dots + live values |
| **OperatorTopBar** | command/search field + alarm state (severity tiles, flood pill) | no identity/connection cells — those live only in `StatusStrip` |
| **WorkspaceTabs** | editor-style tabs for open views | replaces MDI title bars |
| **Panel** | `header (title · sub · actions) + body` | the one grouping unit; no nesting |
| **InspectorPanel** | selected-item detail: big read-out, measurements, controls | limits beside live value (§2) |
| **StatusStrip** | charcoal bottom strip: user/role, connection, latency, endpoint/build, alarm summary | never scrolls away; sole home for persistent context (§8) |
| **SeverityTile / KPI** | count + label, severity-tinted left border | alarm counts on overview |
| **EventRow** | severity tag (bar + label) + unack dot + mono value | colour + second cue (§5) |
| **StatePill** | `Good` / `Active` / quality chip | soft-tinted, text-labelled |
| **Field / Combo / Checkbox** | themed inputs | replace native OS widgets (see §7) |
| **Button** | `primary` (accent), default, `danger` (control send), `disabled` (+reason) | |
| **ConfirmDialog** | two-stage control confirm | present→command diff, audit meta (§7) |

## 6. Iconography

The source set is **[Lucide](https://lucide.dev/)** (ISC — no attribution
required), authored on a 24 px grid at **stroke 2.0**, round caps and joins,
`fill="none"`, `stroke="currentColor"` so one file serves dark, light, and
high-contrast.

| Token | Value |
|---|---|
| Grid / `viewBox` | `0 0 24 24` |
| Stroke width | 2.0 (1.5 is too faint at any size; 2.25 only for a measurably faint 16 px glyph) |
| Rail / toolbar / menus / tabs | 20 px |
| Tree and table rows | 16 px |
| Inspector and dialog headers | 24 px |
| Colour | `currentColor`, tinted by the consumer — never a literal hex in the file |

Stroke width is baked into each SVG at author time; Qt renders the file as
authored and cannot restyle it, so a different stroke means a different file.

- Every actionable icon has a tooltip; icons never carry meaning by colour alone
  (§5 of [`principles.md`](principles.md)). Device state rides on the adjacent
  status dot, not on a recoloured glyph.
- The legacy `client/res/*.png` raster glyphs and the magenta-keyed `*.bmp`
  strips are **superseded** — 16 px only, un-tintable, and licence-encumbered.

> **[`iconography.md`](iconography.md) is authoritative** for the set, the
> delivery pipeline (`qtsvg` → `.qrc` → `QIcon`), the full command→glyph map,
> and the procedure for adding or changing an icon. Read it before touching an
> icon; these tokens are its summary.

## 7. Retiring native OS widgets

Today's Login, Write/Control, and Limits dialogs render as **unstyled native
macOS/Windows widgets** — they ignore the app theme entirely (see
`client/docs/screenshots/`). All modal surfaces must adopt the token set so the
product is visually coherent. The `login` and `control-command` mockups show the
target; use themed `Field`/`Button`/`ConfirmDialog` primitives, not
`QMessageBox`/native `QInputDialog` defaults.

## 8. Reference mockups

Rendered, theme-toggleable HTML under
[`client/docs/ui-mockups/screens/`](../ui-mockups/screens/):

| File | Shows |
|---|---|
| `operator-shell.html` | full shell, all primitives, dark+light |
| `substation-display.html` | single-line mimic in the shell: equipment state, live values, click-to-control |
| `trend.html` | trend workspace: series chips, cursor readout, min/max/avg grid, series inspector |
| `event-journal.html` | event/alarm journal: severity/area filters, acknowledge, event inspector |
| `table-watch.html` | live/historical grid: formulas, quality, sparklines, context menu, row inspector |
| `config-workbench.html` | engineering: hardware tree, tabbed device parameter editor, live device-diagnostics inspector |
| `users-admin.html` | admin: users grid + RBAC role/permission editor, admin-gated |
| `device-protocol-trace.html` | device log: protocol frame trace + decoded APCI/ASDU tree + raw hex |
| `transmission-rules.html` | re-transmission rules grid + rule editor (source → destination IOA) |
| `bulk-create.html` | bulk create-many wizard: stepper, pattern form, live preview + conflict resolution |
| `login.html` | themed sign-in + read-only system preview |
| `control-command.html` | two-stage control/write confirm |

These are the visual source of truth; when a token or component changes, update
the mockup in the same change so the gallery stays honest (same discipline as
`client/docs/screenshots/`).
