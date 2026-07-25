#pragma once

#include <QBitmap>
#include <QIcon>
#include <QPixmap>
#include <QString>

#include <string_view>
#include <vector>

// Slices a horizontal icon strip into `width`-wide tiles.
//
// `resource_path` names a Qt resource (":/res/items.bmp") or a filesystem
// path; `mask_color` is the strip's transparent-key colour. This replaces the
// former Win32 `LoadBitmap(MAKEINTRESOURCE(id))` lookup, so the same strip
// renders on every platform instead of only on Windows.
inline std::vector<QIcon> LoadIcons(std::string_view resource_path,
                                    int width,
                                    QColor mask_color) {
  QPixmap tile{QString::fromUtf8(resource_path.data(),
                                 static_cast<qsizetype>(resource_path.size()))};
  if (tile.isNull() || width <= 0)
    return {};

  tile.setMask(tile.createMaskFromColor(mask_color));

  std::vector<QIcon> icons;
  icons.reserve(static_cast<size_t>((tile.width() + width - 1) / width));
  for (int x = 0; x < tile.width(); x += width)
    icons.emplace_back(QIcon{tile.copy(x, 0, width, tile.height())});
  return icons;
}
