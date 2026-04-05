#pragma once

#include "base/win/scoped_gdi_object.h"

#include <QBitmap>
#include <QIcon>
#include <QPixmap>
#include <qwinfunctions.h>

inline std::vector<QIcon> LoadIcons(unsigned resource_id,
                                    int width,
                                    QColor mask_color) {
  base::win::ScopedBitmap bitmap{
      ::LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(resource_id))};

  QPixmap tile = QtWin::fromHBITMAP(bitmap.get());
  tile.setMask(tile.createMaskFromColor(mask_color));

  std::vector<QIcon> icons;
  // TODO: reserve.
  for (int x = 0; x < tile.width(); x += width)
    icons.emplace_back(QIcon{tile.copy(x, 0, width, tile.height())});
  return icons;
}
