# SCADA Client — Design Language (tokens & components)

> Status: living design document. The client's shared visual vocabulary:
> colour, type, spacing, and the component primitives every screen is built
> from. Rationale is in [`principles.md`](principles.md); layout is in
> [`shell.md`](shell.md).

These token **values are the client's own source of truth**, deliberately kept
numerically identical to the web client's design system so the two products
read as one. When one side changes a token, change the other in the same
initiative. Do not fork the palette.

## 1. Themes

Three themes ship from one token contract, selected by a `data-theme`
equivalent (a Qt palette + QSS variable set):

- **Dark** — the **desktop default** (control-room norm; low glare at night).
- **Light** — mirrors the **web default**; offered on desktop via toggle.
- **High contrast** — accessibility / bright-ambient fallback.

The operator can switch at runtime; the choice persists in the `Profile`.

## 2. Colour tokens

Semantic tokens only — **components never hard-code hex**. Grouped by role.

### Surfaces & text

| Token | Dark | Light | Role |
|---|---|---|---|
| `--bg` | `#07111b` | `#f5f8fb` | app background |
| `--bg-elevated` | `#0b1623` | `#f8fbfe` | sidebars, tab bar, dialog footers |
| `--surface` | `#0f1925` | `#ffffff` | panels, cards, dialogs |
| `--surface-muted` | `#152635` | `#f1f5f9` | table headers, inset fields |
| `--rail-bg` | `#06111b` | `#0d1a27` | Activity bar & status strip (charcoal in **both** themes) |
| `--topbar-bg` | `#0b1623` | `#ffffff` | top context bar |
| `--fg` | `#eef5fb` | `#111827` | primary text |
| `--fg-muted` | `#c3d0db` | `#4b5b6c` | secondary text |
| `--fg-subtle` | `#8fa3b4` | `#6b7b8d` | labels, captions |
| `--border` | `rgba(255,255,255,.12)` | `rgba(15,23,42,.12)` | hairlines |
| `--border-strong` | `rgba(255,255,255,.24)` | `rgba(15,23,42,.22)` | field/control borders |

The **Activity bar and status strip stay charcoal in the light theme too** —
this is the web's deliberate "deep charcoal rail on a light workspace" signature
and the single most recognisable shared cue between the two clients.

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
| **OperatorTopBar** | command/search field + context cluster (plant, server, connection, user) | context always visible (§8) |
| **WorkspaceTabs** | editor-style tabs for open views | replaces MDI title bars |
| **Panel** | `header (title · sub · actions) + body` | the one grouping unit; no nesting |
| **InspectorPanel** | selected-item detail: big read-out, measurements, controls | limits beside live value (§2) |
| **StatusStrip** | charcoal bottom strip: user/role, connection, latency, alarm summary | never scrolls away |
| **SeverityTile / KPI** | count + label, severity-tinted left border | alarm counts on overview |
| **EventRow** | severity tag (bar + label) + unack dot + mono value | colour + second cue (§5) |
| **StatePill** | `Good` / `Active` / quality chip | soft-tinted, text-labelled |
| **Field / Combo / Checkbox** | themed inputs | replace native OS widgets (see §7) |
| **Button** | `primary` (accent), default, `danger` (control send), `disabled` (+reason) | |
| **ConfirmDialog** | two-stage control confirm | present→command diff, audit meta (§7) |

## 6. Iconography

- Line icons, ~1.8 px stroke, 20 px on the rail / 14–15 px inline, monochrome
  tinted by `currentColor`. The existing `client/res/*.png` action glyphs
  (acknowledge, execute, write_manual, printer, table…) remain valid; new chrome
  uses the line set for scalability across DPI.
- Every actionable icon has a tooltip; icons never carry meaning by colour alone.

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
| `debugger.html` | diagnostics: protocol frame trace + decoded APCI/ASDU tree + raw hex |
| `transmission-rules.html` | re-transmission rules grid + rule editor (source → destination IOA) |
| `login.html` | themed sign-in + read-only system preview |
| `control-command.html` | two-stage control/write confirm |

These are the visual source of truth; when a token or component changes, update
the mockup in the same change so the gallery stays honest (same discipline as
`client/docs/screenshots/`).
