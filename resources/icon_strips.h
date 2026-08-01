#pragma once

#include "aui/color.h"

#include <string_view>

// The last horizontal icon strip, sliced through `aui::Tree::LoadIcons`. Kept
// in lock-step with res/client.qrc.
//
// It was a Win32 `BITMAP` resource (IDB_WIN_TYPES) in the client resource
// script until that was dropped; serving it from the Qt resource file makes
// the icons render on every platform rather than only on Windows.
//
// `items.bmp` is gone — every consumer now renders the Lucide glyphs below
// (docs/ux/iconography.md §5.2). This one outlives it only because the
// favourites tree has not been converted.

// Window-type icons for the favourites tree (16-wide tiles).
inline constexpr std::string_view kWindowTypeIconStrip = ":/res/wintypes.bmp";

// Tile width of every strip above, in pixels.
inline constexpr int kIconStripTileWidth = 16;

// The Lucide glyphs that replaced `items.bmp`, indexed by
// `ConfigurationTreeNode::IMAGE_*` so the models' tile-index contract is
// unchanged (docs/ux/iconography.md §5.2). Shared by the address-space trees,
// the table view and the portfolio — the table only ever asks for
// IMAGE_ITEM, and the portfolio for IMAGE_FOLDER / IMAGE_ITEM.
//
// Note the four device tiles and the two subsystem tiles collapse onto one
// glyph each. State moved out of the artwork and onto the status dot the
// models already provide, which is principles.md §5 — colour is never the sole
// carrier — and six fewer assets to keep in sync. `router` for a device and
// `network` for a link/subsystem are the nouns an engineer uses for them.
inline constexpr std::string_view kItemGlyphs[] = {
    ":/icons/folder.svg",    // IMAGE_FOLDER
    ":/icons/variable.svg",  // IMAGE_ITEM
    ":/icons/router.svg",    // IMAGE_DEVICE_RUNNING
    ":/icons/router.svg",    // IMAGE_DEVICE_STOPPED
    ":/icons/network.svg",   // IMAGE_SUBSYSTEM_RUNNING
    ":/icons/network.svg",   // IMAGE_SUBSYSTEM_STOPPED
    ":/icons/router.svg",    // IMAGE_DEVICE
    ":/icons/router.svg",    // IMAGE_DEVICE_DISABLED
};

// Row-glyph size in logical pixels (design-language.md §6: tree and table
// rows are 16 px).
inline constexpr int kTreeGlyphSize = 16;

// The strips are colour-keyed rather than alpha-blended; magenta is the
// transparent pixel.
inline constexpr scada::aui::Rgba kIconStripMaskColor{255, 0, 255};
