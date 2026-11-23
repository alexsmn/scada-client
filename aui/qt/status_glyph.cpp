#include "aui/qt/status_glyph.h"

#include <QApplication>
#include <QFont>
#include <QFontMetrics>
#include <QPainter>
#include <QPen>
#include <QStyle>

#include <algorithm>
#include <cmath>

namespace scada::aui {

int StatusGlyphDiameter(const QFont& font) {
  const int line = QFontMetrics{font}.height();
  const int max = QApplication::style()->pixelMetric(QStyle::PM_SmallIconSize);
  const int wanted = static_cast<int>(std::lround(line * 0.65));
  return std::clamp(wanted, 9, std::max(9, max));
}

QPixmap DrawStatusGlyph(Quality quality,
                        const QColor& color,
                        int diameter,
                        qreal device_pixel_ratio) {
  QPixmap result(QSize{diameter, diameter} * device_pixel_ratio);
  result.setDevicePixelRatio(device_pixel_ratio);
  result.fill(Qt::transparent);

  QPainter painter{&result};
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setRenderHint(QPainter::TextAntialiasing);

  const qreal d = diameter;
  if (quality == Quality::kGood) {
    painter.setPen(Qt::NoPen);
    painter.setBrush(color);
    painter.drawEllipse(QRectF{0.0, 0.0, d, d});
    return result;
  }

  // Ring: stroke centred half a pen inside the bounds so it is not clipped.
  const qreal pen_width = std::max(1.0, d / 7.0);
  const QRectF ring{pen_width / 2, pen_width / 2, d - pen_width, d - pen_width};

  if (quality == Quality::kUncertain) {
    // Left half filled: Qt angles are in 1/16 degree, counter-clockwise from
    // three o'clock, so 90°..270° is the left half.
    painter.setPen(Qt::NoPen);
    painter.setBrush(color);
    painter.drawPie(QRectF{0.0, 0.0, d, d}, 90 * 16, 180 * 16);
  }

  painter.setPen(QPen{color, pen_width});
  painter.setBrush(Qt::NoBrush);
  painter.drawEllipse(ring);

  if (quality == Quality::kBad) {
    QFont font = QApplication::font();
    font.setBold(true);
    font.setPixelSize(std::max(6, static_cast<int>(std::lround(d * 0.72))));
    painter.setFont(font);
    painter.setPen(color);
    painter.drawText(QRectF{0.0, 0.0, d, d}, Qt::AlignCenter,
                     QStringLiteral("?"));
  }
  return result;
}

}  // namespace scada::aui
