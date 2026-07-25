# SCADA Client — UX Design

This directory holds the UX design system for the Telecontrol SCADA desktop
client (Qt). It defines a reshelled **operator workbench** that is visually and
structurally consistent with the web client, grounded in established industrial
HMI standards.

## Direction (agreed)

- **Full reshell** to the web client's workbench language: charcoal **Activity
  bar** → **Explorer** sidebar → editor-style **workspace tabs** → right
  **Inspector** → **status strip**. Not a rewrite of the view/service layers —
  it rides on the existing `main_window/`, `Profile`, registries, and
  `modules/`.
- **Shared design tokens** with the web client, one contract for
  **dark / light / high-contrast**. Desktop **defaults to dark** (control-room
  norm); web defaults to light; both offer the toggle.
- **Operator-first**: live monitoring, alarms/acknowledge, trends, and
  control/write commands are designed first.

## Documents

| Doc | What it covers |
|---|---|
| [`principles.md`](principles.md) | Why the UI behaves as it does — HMI/SCADA standards (High-Performance HMI, ISA-101, ISA-18.2/EEMUA 191, Endsley situational awareness, colour rules) with citations, each traced to a mockup. |
| [`design-language.md`](design-language.md) | The shared vocabulary: colour/type/spacing tokens (exact values), themes, and the component primitives. |
| [`iconography.md`](iconography.md) | The icon set (Lucide, ISC), geometry and size tokens, the `qtsvg` → `.qrc` → `QIcon` pipeline, the full command→glyph map, and how to add or change an icon. |
| [`shell.md`](shell.md) | The reshelled layout, region by region, mapped onto the **existing** client code; navigation rules; open questions. |
| [`vocabulary-parity.md`](vocabulary-parity.md) | Desktop ⇄ web component-name parity matrix (backlog 5.5): shared vocabulary, drift, and reconciliation action items. No cross-repo file dependency. |
| [`backlog.md`](backlog.md) | Operator-first, module-scoped implementation plan (P0→P5) with acceptance lines. |

## Mockups

Rendered, theme-toggleable HTML in
[`../ui-mockups/screens/`](../ui-mockups/screens/) — the visual source of truth:

| File | Shows |
|---|---|
| `operator-shell.html` | The full operator workbench (dark default + light toggle). |
| `substation-display.html` | Single-line mimic display in the shell — equipment state, live values, click-to-control. |
| `trend.html` | Chart-primary trend workspace — series chips, cursor readout, min/max/avg grid, series inspector. |
| `event-journal.html` | Filterable event/alarm journal — severity filters, acknowledge, area filter, event inspector. |
| `table-watch.html` | Live + historical operator grid — formulas/NodeIds, quality, embedded sparklines, Qt-shaped context menu, row inspector. |
| `config-workbench.html` | Engineering surface — device/hardware tree, tabbed device parameter editor (IEC 60870 fields, address map), live device-diagnostics inspector. |
| `users-admin.html` | Admin surface — users grid + RBAC role/permission editor with inherited-vs-explicit grants, admin-gated. |
| `debugger.html` | Diagnostics — protocol frame trace (IEC 60870) with decoded APCI/ASDU tree + raw hex, direction/error coding. |
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
- The web client codifies the same doctrine as a shipping product; this desktop
  work keeps the **design language and token values** aligned (shared
  vocabulary, no cross-repo code dependency).

## Maintenance

Treat these as living documents. When a token, component, or shell region
changes: update the relevant doc **and** the affected mockup in the same change,
and regenerate the touched `client/docs/screenshots/` image once the code lands
— the same discipline the screenshot generator already follows.
