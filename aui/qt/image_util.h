#pragma once

#include "aui/color.h"

#include <QBitmap>
#include <QColor>
#include <QFile>
#include <QIcon>
#include <QPainter>
#include <QPalette>
#include <QPixmap>
#include <QString>

#include <lunasvg.h>

#include <span>
#include <string_view>
#include <vector>

// Renders one Lucide SVG resource into a `size`-square icon painted in `tint`.
//
// The files carry `stroke="currentColor"`, which Qt's SVG renderer has no
// notion of — it resolves to black. So the glyph is rendered to a transparent
// pixmap and recoloured through it (`SourceIn` keeps the stroke's coverage,
// including its antialiasing, and replaces the colour). That is what
// docs/client/ux/iconography.md §4 means by "tint is applied by the consumer,
// not the file": one asset serves dark, light and high-contrast.
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
  if (size <= 0)
    return {};

  // The glyphs are Qt resources (":/..."), which lunasvg cannot open by path,
  // so the bytes are read out first. lunasvg is used rather than Qt's own SVG
  // module because qtsvg's vcpkg build shells out to `xcodebuild` and so needs
  // a full Xcode; lunasvg is pure C++ and was the only Qt dependency standing
  // between the client and a standalone build from its export.
  QFile file{path};
  if (!file.open(QIODevice::ReadOnly))
    return {};
  const QByteArray svg = file.readAll();
  const auto document = lunasvg::Document::loadFromData(
      svg.constData(), static_cast<size_t>(svg.size()));
  if (!document)
    return {};

  const qreal dpr = device_pixel_ratio > 0 ? device_pixel_ratio : 1.0;
  const int px = qRound(size * dpr);
  QPixmap pixmap{QSize{px, px}};
  pixmap.setDevicePixelRatio(dpr);
  pixmap.fill(Qt::transparent);

  // Rendered at device pixels, then drawn into the logical rect: same crispness
  // guarantee the QSvgRenderer path gave, kept explicit now that the rasteriser
  // no longer knows about Qt's device pixel ratio.
  lunasvg::Bitmap bitmap = document->renderToBitmap(px, px);
  if (!bitmap.valid())
    return {};
  const QImage glyph{bitmap.data(), px, px,
                     static_cast<qsizetype>(bitmap.stride()),
                     QImage::Format_ARGB32_Premultiplied};

  // The destination is in *logical* units, not device pixels: QPainter reads
  // the pixmap's device pixel ratio and scales by it itself. Passing `px` here
  // multiplies by the ratio a second time, so on a HiDPI display the glyph is
  // drawn dpr times too large and clipped against the pixmap — which is how a
  // 24 px toolbar icon came out as a zoomed crop that overflowed its button.
  const QRectF target{0, 0, static_cast<qreal>(size), static_cast<qreal>(size)};

  QPainter painter{&pixmap};
  painter.setRenderHint(QPainter::Antialiasing);
  painter.drawImage(target, glyph);
  painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
  painter.fillRect(target, tint);
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
// (docs/client/ux/iconography.md §5.2) — so they take a muted text colour
// rather than competing with the label they sit beside.
inline scada::aui::Color GlyphTintFor(const QPalette& palette) {
  QColor tint = palette.color(QPalette::Text);
  tint.setAlphaF(0.7);
  return scada::aui::Rgba{static_cast<unsigned char>(tint.red()),
                          static_cast<unsigned char>(tint.green()),
                          static_cast<unsigned char>(tint.blue()),
                          static_cast<unsigned char>(tint.alpha())};
}
