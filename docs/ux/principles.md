# SCADA Client — UX Principles

> Status: living design document. Establishes *why* the Telecontrol SCADA
> client looks and behaves the way the mockups propose. Screen-level specs
> live in [`shell.md`](shell.md); the token/component vocabulary lives in
> [`design-language.md`](design-language.md); the rollout plan lives in
> [`backlog.md`](backlog.md).

## 0. Audience and context

The client is an **operator-facing supervisory interface** for a power-systems
telecontrol network (IEC 60870-5-104, IEC 61850, Modbus, OPC UA). Its primary
user is a control-room operator who works **24/7**, monitors live telemetry,
responds to alarms, and issues control commands to physical equipment. A
secondary user is an engineer configuring devices, limits, and transmission
rules.

Everything below is anchored in established industrial-HMI practice. These are
not aesthetic preferences — they are the accepted standards for interfaces
where a slow or wrong operator response has physical consequences.

## 1. Design for the abnormal, not the steady state

The core job of the HMI is **fast, correct operator response when something goes
wrong** — not to look busy while everything is fine. A normally-running plant
should look calm and nearly colourless, so the eye is drawn only to what needs
attention.

- Use **low-saturation, neutral backgrounds**. Reserve bright, saturated colour
  (red/amber) **exclusively for abnormal conditions and alarms**.
- Do not decorate healthy states with strong green/red "running/stopped"
  colour — it competes with real alarms for the operator's attention.
- Muted tones also cut glare and reflection in a lit control room.

This is the central tenet of the **High Performance HMI** approach; our
severity palette (see [`design-language.md`](design-language.md)) is deliberately
desaturated in the light theme for exactly this reason.

> Sources: [The High Performance HMI Handbook (Hollifield et al., PAS)](https://www.amazon.com/High-Performance-HMI-Handbook/dp/0977896919) · [Corso Systems — HP HMI Handbook, Part 1](https://corsosystems.com/posts/the-high-performance-hmi-handbook-and-you-part-1) · [IDC — The High Performance HMI (PDF)](https://www.idc-online.com/technical_references/pdfs/electronic_engineering/The_high_performance.pdf)

## 2. Support the three levels of situational awareness

Endsley's model says operator awareness is built in three stages; the UI must
serve each one:

1. **Perception** — every critical value must be *findable* with adequate
   contrast. Clutter, poor contrast, and missing data cause perception
   failures. → dense-but-scannable layout, tabular numbers, quality/staleness
   marks on every value.
2. **Comprehension** — a value has meaning only *against its normal range*.
   Show limits, deviation, and grouping so the operator does not have to compute
   "is this bad?" in their head. → the Inspector shows warning/alarm limits next
   to the live value; out-of-limit values recolour.
3. **Projection** — the operator must see *where a value is heading* to act
   before the alarm fires. → trends and rate-of-change are first-class, not a
   separate destination (§4).

> Sources: [Endsley — Toward a Theory of Situation Awareness](https://www.researchgate.net/publication/210198492_Endsley_MR_Toward_a_Theory_of_Situation_Awareness_in_Dynamic_Systems_Human_Factors_Journal_371_32-64) · [Situation awareness — Wikipedia](https://en.wikipedia.org/wiki/Situation_awareness)

## 3. Alarms follow a rationalized, prioritized discipline

Alarm handling is the highest-stakes part of the UI and has its own standards
(ANSI/ISA-18.2, EEMUA 191, IEC 62682).

- **Every alarm demands a unique operator action.** If there is no defined
  response, it is not an alarm — it belongs in an informational log, not the
  alarm surface. We separate the **event journal** (history, all severities)
  from the **active-alarm surface** (unacknowledged, actionable).
- **Prioritize by consequence and time-to-respond.** Keep priority levels few
  and meaningful; map them to one consistent colour ramp everywhere
  (Critical / High / Medium / Info).
- Provide **acknowledge**, and design for **shelve/suppress** with logging and
  access control, so an alarm flood (>10 alarms in 10 minutes) during
  startup/upset does not bury the one alarm that matters.
- Target manageable rates as a design goal: steady-state ~1 alarm per 10 min
  per operator is acceptable; a sustained rate above 1/min is not. The UI
  should make flood conditions *obvious* (counts, grouping) rather than
  scrolling them past.

> Sources: [ANSI/ISA-18.2-2016 (PDF)](https://18817087.s21i.faiusr.com/61/ABUIABA9GAAgyZfj5AUozIu7wwI.pdf) · [ProcessVue — ISA-18.2 / EEMUA 191 guidelines](https://www.processvue.com/resources/alarm-management-guidelines/) · [IEC 62682](https://webstore.iec.ch/publication/59991) · [HumanFactors101 — alarm performance](https://humanfactors101.com/topics/alarms/)

## 4. Trends and analog context over raw numbers

Humans read analog depictions intuitively; raw digits require interpretation.

- Prefer **analog/trend representations with limit markings** so value,
  deviation, and proximity-to-limit read in a ~2-second glance.
- Make **trends first-class**: a dominant trend panel on the operator overview,
  and embedded mini-trends/sparklines beside key values — not only on a separate
  trend page. Recent history beside a live value is what enables *projection*.
- Support **multi-pane / overlaid time-series** with consistent scaling for
  correlating related variables during an upset. (The client already has a
  strong multi-pane graph; the work is presentation polish and embedding.)

> Sources: [Inductive Automation — Optimizing Your HMI](https://inductiveautomation.com/resources/article/design-like-a-pro-optimizing-your-hmi) · [Ignition — High Performance HMI Techniques](https://docs.inductiveautomation.com/docs/8.3/ignition-modules/vision/common-tasks-in-vision/high-performance-hmitechniques)

## 5. Colour is never the only signal

~8% of men have red-green colour-vision deficiency, and control-room lighting
varies.

- **Pair every colour with a second cue** — shape, icon, text label, or
  position. A red alarm row also carries a severity label and an unacknowledged
  dot; a bad-quality value also carries a `?`/text mark.
- Use **one severity palette system-wide**, applied identically on every
  surface (tree, event journal, alarm strip, inspector, status bar).
- The palette must stay legible in **light, dark, and high-contrast** themes.

> Sources: [Aufait UX — HMI colour coding](https://www.aufaitux.com/blog/hmi-color-coding-psychology-safety-design/) · [Control Engineering — high-performance HMIs](https://www.controleng.com/developing-high-performance-hmis-enhancing-interface-effectiveness/)

## 6. A consistent, hierarchical display structure

ANSI/ISA-101 asks for a documented **HMI style guide** and a **defined display
hierarchy** applied consistently across every screen, rather than one-off
layouts.

- **This document set is that style guide.** New screens consume the shared
  tokens and shell rather than inventing chrome.
- Follow the **Level 1–4 hierarchy**: Level 1 = plant/area overview (health at a
  glance); Level 2 = unit/feeder control; Level 3 = detail/sub-system; Level 4 =
  diagnostics/faceplates. The reshelled navigation (Activity bar → Explorer →
  workspace tabs → Inspector) maps directly onto these levels.
- Keep shell chrome — Activity bar, top context bar, status strip — **identical
  across every section**.

> Sources: [ISA-101 Standards Series](https://www.isa.org/standards-and-publications/isa-standards/isa-101-standards) · [ANSI/ISA-101.01-2015](https://www.isa.org/products/ansi-isa-101-01-2015-human-machine-interfaces-for) · [ISO 11064 — control-centre ergonomics](https://www.iso.org/obp/ui/#iso:std:iso:11064:-1:ed-1:v1:en)

## 7. Control actions are deliberate and auditable

Issuing a command to physical equipment is irreversible in the field.

- Use a **two-stage confirm** for control commands: enter/select, then review a
  summary (present → command, operator identity, command model, timestamp)
  before send. See the `control-command` mockup.
- Always show **who** the action is logged against and **what** the effect is.
- **Disable, don't hide** controls the current user or capability can't perform,
  and state the reason (e.g. "admin only") — this preserves the operator's model
  of what the system can do. Never imply an unsupported control is live.

## 8. Keep operator context permanently visible

At all times the operator can see: **plant/site, server health, connection
state, current user, active-alarm count and highest severity, and current
selection**. These live in the top context bar and the bottom status strip and
never scroll away.

Permanently visible means **visible once**, not visible twice. Each of these
facts has exactly one home: alarm counts and severity in the top bar (where
colour change must be pre-attentive), identity and connection in the status
strip (steady, glanceable). Duplicating a fact across both bars does not make it
more persistent — it spends chrome and gives the operator a second place to
check. See `shell.md` §2.2/§2.7 for the split and the reason it was corrected.

## 9. Consistency with the web client

The web client (`web/`) already codifies this doctrine as a VS Code-style
operator workbench with a mature, theme-aware token system. The desktop client
adopts **the same design language and the same token values** so an operator
moving between desktop and browser sees one product. Where the two must differ,
the difference is **platform-idiomatic, not stylistic**: the desktop keeps
native window management, dockable panes, and multi-window profiles; the web
keeps browser navigation and responsive collapse. The **default theme differs
by context** — desktop defaults to **dark** (control-room norm), web defaults to
**light** — but both ship the same light/dark/high-contrast token sets and the
same toggle.

---

### Principle → mockup traceability

| Principle | Where it shows up |
|---|---|
| §1 Design for abnormal | Desaturated severity ramp; calm neutral surfaces (`design-language.md`) |
| §2 Situational awareness | Inspector limits beside live value; quality marks; trend (`operator-shell`) |
| §3 Alarm discipline | Separate active-alarm strip vs event journal; ack-all; severity counts (`operator-shell`) |
| §4 Trends first-class | Dominant trend panel + legend with current values (`operator-shell`) |
| §5 Colour + second cue | Severity label + dot + colour together; `?` on bad quality |
| §6 Hierarchy + style guide | Activity bar / Explorer / tabs / Inspector shell (`shell.md`) |
| §7 Deliberate control | Two-stage confirm dialog (`control-command`) |
| §8 Persistent context | Top context bar + bottom status strip (`operator-shell`, `login`) |
| §9 Web consistency | Shared tokens; dark-default desktop vs light-default web |
