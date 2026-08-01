#pragma once

#include "aui/color.h"

#include <QBitmap>
#include <QColor>
#include <QPalette>
#include <QIcon>
#include <QPainter>
#include <QPixmap>
#include <QString>
#include <QSvgRenderer>

#include <span>
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

// Renders one Lucide SVG resource into a `size`-square icon painted in `tint`.
//
// The files carry `stroke="currentColor"`, which Qt's SVG renderer has no
// notion of — it resolves to black. So the glyph is rendered to a transparent
// pixmap and recoloured through it (`SourceIn` keeps the stroke's coverage,
// including its antialiasing, and replaces the colour). That is what
// docs/ux/iconography.md §4 means by "tint is applied by the consumer, not the
// file": one asset serves dark, light and high-contrast.
//
// Rendered at the device pixel ratio, so a 16 px row glyph stays crisp on a
// HiDPI display instead of being upscaled from 16 physical pixels the way the
// retired bitmap strips were.
inline QIcon LoadTintedGlyph(std::string_view resource_path,
                             int size,
                             QColor tint,
                             qreal device_pixel_ratio = 1.0) {
  const QString path = QString::fromUtf8(
      resource_path.data(), static_cast<qsizetype>(resource_path.size()));
  QSvgRenderer renderer{path};
  if (!renderer.isValid() || size <= 0)
    return {};

  const qreal dpr = device_pixel_ratio > 0 ? device_pixel_ratio : 1.0;
  QPixmap pixmap{QSize{size, size} * dpr};
  pixmap.setDevicePixelRatio(dpr);
  pixmap.fill(Qt::transparent);

  QPainter painter{&pixmap};
  painter.setRenderHint(QPainter::Antialiasing);
  renderer.render(&painter, QRectF{0, 0, static_cast<qreal>(size),
                                   static_cast<qreal>(size)});
  painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
  painter.fillRect(QRectF{0, 0, static_cast<qreal>(size),
                          static_cast<qreal>(size)},
                   tint);
  painter.end();

  return QIcon{pixmap};
}

// Renders a whole glyph set, preserving index order so it can replace a sliced
// bitmap strip without changing the models' "tile index" contract. An entry
// whose resource is missing yields a null QIcon, which consumers already treat
// as "no icon" rather than drawing a placeholder.
inline std::vector<QIcon> LoadTintedGlyphs(
    std::span<const std::string_view> resource_paths,
    int size,
    QColor tint,
    qreal device_pixel_ratio = 1.0) {
  std::vector<QIcon> icons;
  icons.reserve(resource_paths.size());
  for (std::string_view path : resource_paths)
    icons.push_back(LoadTintedGlyph(path, size, tint, device_pixel_ratio));
  return icons;
}

// The colour row glyphs are rendered in, from a live palette.
//
// Row glyphs mark *kind*, never state — state rides the status dot
// (docs/ux/iconography.md §5.2) — so they take a muted text colour rather than
// competing with the label they sit beside.
inline scada::aui::Color GlyphTintFor(const QPalette& palette) {
  QColor tint = palette.color(QPalette::Text);
  tint.setAlphaF(0.7);
  return scada::aui::Rgba{static_cast<unsigned char>(tint.red()),
                          static_cast<unsigned char>(tint.green()),
                          static_cast<unsigned char>(tint.blue()),
                          static_cast<unsigned char>(tint.alpha())};
}
