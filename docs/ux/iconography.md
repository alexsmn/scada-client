# SCADA Client — Iconography

> Status: living design document. Owns the client's icon **source set, geometry,
> delivery pipeline, and update procedure**. The size/stroke/tint tokens
> summarised in [`design-language.md`](design-language.md) §6 are authoritative
> here. Rationale for the visual rules: [`principles.md`](principles.md) §5
> (colour is never the sole carrier of meaning).
>
> Last verified against code: 2026-07-25.

## 1. Why this document exists

The client shipped three unrelated icon families through three unrelated
mechanisms, and none of them survive a reshell:

| Path | Assets | Problem |
|---|---|---|
| `kIconResources` in [`ui/qt/client_utils_qt.cpp`](../../ui/qt/client_utils_qt.cpp) | 17 × 16×16 PNG under `res/` | Silk-lineage raster; 16 px only |
| Icon strips via [`resources/icon_strips.h`](../../resources/icon_strips.h) | `res/items.bmp`, `res/wintypes.bmp` | Win95-era magenta-keyed 16-colour bitmaps |
| `res/settings/settings*.png` | 8 pre-rendered depth/size variants | Flaticon download; different family again |

Three defects follow, all of them structural rather than cosmetic:

1. **No HiDPI.** These are 16 px rasters with no @2x. On macOS and on Windows
   at 125 %/150 % Qt bicubic-upscales them into mush. The reshell targets
   crisp chrome at arbitrary scale factors.
2. **Light-theme art on a dark-default shell.** The desktop default theme is
   dark ([`design-language.md`](design-language.md) §1). Gold bells and green
   spreadsheets cannot be tinted by `currentColor`; they stay light-mode
   islands.
3. **No provenance record.** The repo carries no `NOTICE`; the only trace is a
   one-line `res/settings/source.txt`. Both upstream families carry an
   attribution obligation (see §7).

Meanwhile [`main_window/activity_bar_qt.cpp`](../../main_window/activity_bar_qt.cpp)
already hand-draws its rail glyphs with `QPainter` at a 1.8 px round-cap stroke
— i.e. someone reimplemented Feather/Lucide geometry in C++ because there was no
set to draw from. This document gives that set a name so the two paths converge.

## 2. The source set: Lucide

**[Lucide](https://lucide.dev/)** — ISC licensed, ~1,750 icons, 24 px grid,
2 px round-cap/round-join stroke, `stroke="currentColor"`.

Chosen because:

- **ISC — no attribution required.** Compatible with the client's Apache-2.0
  licensing with no downstream obligation (contrast §7).
- **Geometry already matches the shell.** 24/2 round-cap is what
  `DrawSectionGlyph` draws by hand today.
- **Coverage is complete for this client.** Every action icon and every tree
  tile in §5 maps to a stock Lucide glyph — no gaps, no commissioning.
- **`currentColor` throughout**, so one file serves dark, light, and
  high-contrast.

Evaluated and rejected: **Material Symbols** (Apache-2.0, exact licence match,
but heavier/filled and visually foreign to the workbench), **Tabler** (MIT, same
24/2 geometry, larger but less curated — a viable drop-in substitute if Lucide
ever lacks a glyph), **Font Awesome Free** (attribution required — same problem
we are leaving behind).

> Do not mix sets. A glyph Lucide lacks is drawn *in Lucide's geometry* (§6,
> step 5), not borrowed from another family.

## 3. Geometry and size tokens

| Token | Value |
|---|---|
| Grid / `viewBox` | `0 0 24 24` |
| Stroke width | **2.0** |
| Line caps / joins | round / round |
| Fill | `none` |
| Colour | `currentColor` — never a literal hex |

Rendered sizes:

| Context | Size | Notes |
|---|---|---|
| Activity bar rail | 20 px | matches `kIconSize` in `activity_bar_qt.cpp` |
| Toolbar, menus, workspace tabs | 20 px | |
| Tree and table rows | 16 px | dense rows; verified legible at stroke 2.0 |
| Inspector and dialog headers | 24 px | native grid, no scaling |

**One file per glyph, at stroke 2.0, serves all four sizes.** Stroke width is
baked into the SVG at author time — Qt renders the file as-is and cannot restyle
it — so a different stroke means a *different file*. Only introduce a `-dense`
variant at stroke 2.25 when a specific glyph is measurably faint at 16 px;
1.5 is too faint at every size and must not be used.

Colour rules (from [`principles.md`](principles.md) §5): an icon never carries
meaning by colour alone — pair every tint with shape, position, or text. Device
state is the worked example: the glyph stays neutral and state is carried by the
adjacent status dot that
[`modules/configuration/devices/device_state_color.cpp`](../../modules/configuration/devices/device_state_color.cpp)
already computes. Every actionable icon has a tooltip.

## 4. Delivery pipeline

```
client/res/icons/<name>.svg          ← verbatim Lucide source, one file per glyph
        └─ listed in client/res/client.qrc under the "/icons" prefix
             └─ QIcon{":/icons/<name>.svg"} via LoadPixmap() / direct construction
                  └─ rendered by Qt's qsvgicon icon-engine plugin (Qt Svg module)
```

**The `qtsvg` dependency is what makes this work.** `QIcon` cannot load an SVG
without Qt's SVG icon-engine plugin, which ships in the Qt Svg module, not
`qtbase`. It is declared in both manifests:

- [`client/vcpkg.json`](../../vcpkg.json) — `"qtsvg"`
- superproject [`vcpkg.json`](../../../vcpkg.json) — under the `client`
  feature, `"platform": "windows | osx"` (mirroring `qtbase`)

Still to wire when the first SVG icon lands (deliberately *not* done ahead of
time, so the configure does not hard-fail on Qt installs that predate the
manifest change):

- `find_package(Qt6 REQUIRED COMPONENTS Widgets Svg)` in
  [`client/CMakeLists.txt`](../../CMakeLists.txt) and a `Qt6::Svg`
  link, so `windeployqt`/`macdeployqt` pick the plugin up.
- Static builds additionally need the plugin imported explicitly
  (`Qt6::QSvgIconPlugin`).

Tint is applied by the consumer, not the file: recolour via `QPalette` /
`ThemeTokens` the way `SectionIcon` does, so one asset serves every theme.

## 5. Icon map

The target mapping. Where the "Current" column is populated the swap has **not
yet landed** — see §8.

### 5.1 Action icons

Keyed by command id in `kIconResources`
([`ui/qt/client_utils_qt.cpp`](../../ui/qt/client_utils_qt.cpp)). That table is
keyed on the *raw numeric* id and `common_resources.h` reuses numbers across
unrelated symbols — re-read the warning above the table before adding a row.

| Command id | Meaning | Current | Lucide |
|---|---|---|---|
| `ID_GRAPH_VIEW` | trend / graph view | `chart_curve.png` | `chart-spline` |
| `ID_TABLE_VIEW` | table view | `table.png` | `table` |
| `IDB_SUMMARY` | summary view | `summary.png` | `clipboard-list` |
| `IDB_TIMED_DATA` | historical (timed) data | `doc_table.png` | `file-clock` |
| `ID_EVENT_VIEW` | event / alarm view | `event_view.png` | `triangle-alert` |
| `IDB_OPEN_EVENTS` | event journal | `events3.png` | `logs` |
| `IDB_RECORD_EDITOR` | record editor | `record_editor.png` | `file-pen` |
| `IDB_PRINTER` | print | `printer.png` | `printer` |
| `IDB_WRITE` | control / write | `execute.png` | `zap` |
| `IDB_WRITE_MANUAL` | manual input | `write_manual.png` | `pencil-line` |
| `IDB_UNLOCK` | unlock item | `unlock.png` | `lock-open` |
| `IDB_COPY` | copy | `copy.png` | `copy` |
| `IDB_PASTE` | paste | `paste.png` | `clipboard-paste` |
| `IDB_DELETE` | delete | `delete.png` | `trash-2` |
| `IDB_ACKNOWLEDGE_ALL` | acknowledge all | `acknowledge_all.png` | `check-check` |
| `ID_MODUS_VIEW` | mimic display (Modus) | `display.png` | `workflow` |
| `ID_APPLICATION` | settings / application | `settings/settings64-32bit.png` | `settings` |
| — (direct `QIcon`) | device, in [`aui/qt/item_delegate.cpp`](../../aui/qt/item_delegate.cpp) | `device.png` | `radio-tower` |

### 5.2 Tree and table tiles

The strips are horizontal bitmaps sliced by tile index
([`aui/qt/image_util.h`](../../aui/qt/image_util.h) `LoadIcons`), with magenta
(255,0,255) as the transparency key. Indices come from the `IMAGE_*` enum in
[`modules/configuration/tree/configuration_tree_node.h`](../../modules/configuration/tree/configuration_tree_node.h)
and from `FavouritesWindowNode::GetIcon`.

| Strip / index | Constant | Meaning | Lucide |
|---|---|---|---|
| `items` 0 | `IMAGE_FOLDER` | object / data folder | `folder` |
| `items` 1 | `IMAGE_ITEM` | OPC UA variable / data item | `variable` |
| `items` 2 | `IMAGE_DEVICE_RUNNING` | device online | `router` + good dot |
| `items` 3 | `IMAGE_DEVICE_STOPPED` | device offline | `router` + bad dot |
| `items` 4 | `IMAGE_SUBSYSTEM_RUNNING` | channel online | `network` + good dot |
| `items` 5 | `IMAGE_SUBSYSTEM_STOPPED` | channel offline | `network` + bad dot |
| `items` 6 | `IMAGE_DEVICE` | device, state unknown | `router` |
| `items` 7 | `IMAGE_DEVICE_DISABLED` | device disabled | `router` + uncertain dot |
| `wintypes` 0 | — | favourite: table window | `table` |
| `wintypes` 1 | — | favourite: graph window | `chart-spline` |
| `wintypes` 2 | — | favourite: folder | `folder` |

Note the four device rows collapse onto **one** glyph. State moves out of the
artwork and onto the status dot `HardwareTreeModel::GetStatusColor` already
provides, which is both the §3 colour rule and one fewer asset to keep in sync.
This is why replacing the strips is a larger job than replacing the action
icons: it converts `LoadIcons`' "tile index" contract into "glyph + tint".

**The Objects explorer (`"Struct"`) has opted out of tree icons entirely** —
`ObjectTreeModel::ObjectTreeNode::GetIcon` returns `scada::aui::kNoIcon`. Every
row there is a container or a data item, a distinction the twisty and the
indentation already make, so a glyph could only repeat it; the one thing that
varies between rows, live quality, rides the status dot. This is what
[`ui-mockups/screens/operator-shell.html`](../ui-mockups/screens/operator-shell.html)
shows: its rows carry no icon element at all.

The engineering trees keep theirs, and the mockups agree — the hardware tree in
[`config-workbench.html`](../ui-mockups/screens/config-workbench.html) mixes
links, devices and signals at one level, where a kind glyph still answers a
question. Note that even there, *device* rows use a status dot and no icon. So
the rule across both surfaces is: **state is a dot, never artwork; an icon
marks kind, and only where kind is not already obvious.**

That leaves the strips consumed by `"Nodes"`, `"Subsystems"`, the table view
and the portfolio — the conversion above still has to happen, on a smaller
surface.

## 6. How to add or change an icon

1. **Pick a stock Lucide name.** Search <https://lucide.dev/icons/>. Prefer an
   existing glyph over a variant; prefer the noun the operator would say.
2. **Download the source verbatim** into `client/res/icons/<name>.svg`:

   ```bash
   curl -sL -o client/res/icons/<name>.svg \
     https://raw.githubusercontent.com/lucide-icons/lucide/main/icons/<name>.svg
   ```

   Do not hand-edit the geometry. Do not replace `currentColor` with a hex
   value — that is what breaks theming.
3. **Register it** in [`client/res/client.qrc`](../../res/client.qrc) under the
   `/icons` prefix, keeping the list alphabetical.
4. **Wire it**: add the `{command_id, ":/icons/<name>.svg"}` row to
   `kIconResources`, or construct the `QIcon` directly at the use site. Add the
   row to §5.1 of this document in the same change.
5. **If Lucide genuinely lacks the glyph**, draw it on the §3 contract —
   `0 0 24 24`, stroke 2.0, round caps and joins, `fill="none"`,
   `stroke="currentColor"` — and file it alongside the stock icons with a
   comment naming it as bespoke. Do not import from another icon family.
6. **Verify at the real sizes.** Render 16/20/24 px against both the dark and
   light background tokens and look at the result; a glyph that reads at 24 px
   can dissolve at 16. The headless `client_screenshot_generator`
   ([`../screenshots.md`](../screenshots.md)) is the check for anything already
   on screen — HTML mockups do not match Qt's rendering.
7. **Refresh the affected screenshots and the manual** if the icon is
   user-visible, per `client/CLAUDE.md` → "Doc screenshots and the web manual".

To change the **stroke width** globally: it is baked into every SVG's
`stroke-width` attribute, so a global change means re-downloading the set with
the new value and updating the §3 table. Do not attempt to restyle at runtime;
Qt's SVG icon engine renders the file as authored.

## 7. Licensing and attribution

- **Lucide is ISC.** No attribution is required in the product UI. The ISC
  notice must nonetheless travel with copies of the files — keep
  `client/res/icons/LICENSE` alongside them. A subset of Lucide is
  Feather-derived and MIT (© 2013-present Cole Bemis); the same applies.
- **What we are leaving behind carries obligations we never met.** The Silk
  lineage is CC-BY 2.5 (attribution + link required) and Flaticon's free tier
  requires credit in an application's about/credits screen. Neither appears
  anywhere in the repo.
- **When the swap lands, add a `NOTICE`** recording the icon set, its licence,
  and its version, and drop the retired assets rather than leaving them
  unreferenced in `res/`. Retiring the old files is what actually closes the
  gap; adding attribution for icons we no longer ship is not the goal.

## 8. Migration state

| Step | State |
|---|---|
| `qtsvg` in both vcpkg manifests | ✅ done (2026-07-25) |
| Set, geometry, and map agreed | ✅ this document |
| CMake `Qt6::Svg` component + link | ⬜ lands with the first SVG icon (§4) |
| Action icons (§5.1) — 18 glyphs, one table | ⬜ |
| Tree strips (§5.2) — needs `LoadIcons` → glyph+tint | ⬜ larger, do second |
| `activity_bar_qt.cpp` hand-drawn glyphs → SVG assets | ⬜ optional; converges the two paths |
| `NOTICE` + retire `res/*.png`, `res/*.bmp`, `res/settings/` | ⬜ with the last swap |

The **application icon** (`res/client.ico`, `res/client.icns`) is deliberately
out of scope. It is a brand asset, not chrome, and is not served by a generic
icon set — it needs a purpose-drawn multi-resolution redraw.
