#include "modules/table/qt/sparkline_delegate.h"

#include "aui/qt/theme_qt.h"
#include "modules/table/sparkline.h"

#include <QPainter>
#include <QPolygonF>

SparklineDelegate::SparklineDelegate(SeriesProvider series_provider,
                                     QObject* parent)
    : QStyledItemDelegate{parent},
      series_provider_{std::move(series_provider)} {}

SparklineDelegate::~SparklineDelegate() = default;

void SparklineDelegate::paint(QPainter* painter,
                              const QStyleOptionViewItem& option,
                              const QModelIndex& index) const {
  // Background/selection first; the cell carries no text.
  QStyledItemDelegate::paint(painter, option, index);

  if (!series_provider_)
    return;

  const std::vector<double> series = series_provider_(index.row());
  const QRect rect = option.rect.adjusted(4, 0, -4, 0);
  const std::vector<SparklinePoint> points = ComputeSparklinePoints(
      series, static_cast<float>(rect.width()),
      static_cast<float>(rect.height()), /*padding=*/3.0f);
  if (points.size() < 2)
    return;

  QPolygonF polyline;
  polyline.reserve(static_cast<int>(points.size()));
  for (const SparklinePoint& point : points)
    polyline << QPointF(rect.x() + point.x, rect.y() + point.y);

  painter->save();
  painter->setRenderHint(QPainter::Antialiasing);
  painter->setPen(QPen{scada::aui::ActiveThemeTokens().accent, 1.2,
                       Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin});
  painter->drawPolyline(polyline);
  painter->restore();
}
