#pragma once

#include "aui/color.h"

#include <string_view>

// Qt resource paths of the horizontal icon strips the tree/table views slice
// through `aui::Tree::LoadIcons` / `aui::Table::LoadIcons`. Kept in lock-step
// with res/client.qrc.
//
// These were Win32 `BITMAP` resources (IDB_ITEMS / IDB_WIN_TYPES) in the
// client resource script until it was dropped; serving them from the Qt
// resource file makes the icons render on every platform rather than only on
// Windows.

// Node-class icons for address-space trees and tables (16x16 tiles).
inline constexpr std::string_view kItemIconStrip = ":/res/items.bmp";

// Window-type icons for the favourites tree (16-wide tiles).
inline constexpr std::string_view kWindowTypeIconStrip = ":/res/wintypes.bmp";

// Tile width of every strip above, in pixels.
inline constexpr int kIconStripTileWidth = 16;

// The strips are colour-keyed rather than alpha-blended; magenta is the
// transparent pixel.
inline constexpr scada::aui::Rgba kIconStripMaskColor{255, 0, 255};
