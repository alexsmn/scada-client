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
| [`shell.md`](shell.md) | The reshelled layout, region by region, mapped onto the **existing** client code; navigation rules; open questions. |
| [`backlog.md`](backlog.md) | Operator-first, module-scoped implementation plan (P0→P5) with acceptance lines. |

## Mockups

Rendered, theme-toggleable HTML in
[`../ui-mockups/screens/`](../ui-mockups/screens/) — the visual source of truth:

| File | Shows |
|---|---|
| `operator-shell.html` | The full operator workbench (dark default + light toggle). |
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
