#pragma once

#include <string_view>

// Lucide glyph tables for the tree and table row icons. The magenta-keyed
// bitmap strips these replaced (`items.bmp`, `wintypes.bmp`) are gone, and with
// them the last Win95-era assets in the client — see
// docs/client/ux/iconography.md §5.2 for the mapping and the reasoning.

// The Lucide glyphs that replaced `items.bmp`, indexed by
// `ConfigurationTreeNode::IMAGE_*` so the models' tile-index contract is
// unchanged. Shared by the address-space trees, the table view and the
// portfolio — the table only ever asks for IMAGE_ITEM, and the portfolio for
// IMAGE_FOLDER / IMAGE_ITEM.
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

// The Lucide glyphs that replaced `wintypes.bmp`, indexed by
// `FavouritesWindowNode::GetIcon` — a favourite is a saved *window*, so the
// glyph names the kind of view it reopens. A favourite of any other view type
// returns -1 and shows none, which is why there is no default entry.
inline constexpr std::string_view kWindowTypeGlyphs[] = {
    ":/icons/table.svg",         // table window
    ":/icons/chart-spline.svg",  // graph window
    ":/icons/folder.svg",        // folder
};

// Row-glyph size in logical pixels (design-language.md §6: tree and table
// rows are 16 px).
inline constexpr int kTreeGlyphSize = 16;
