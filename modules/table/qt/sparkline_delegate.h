#pragma once

#include <QStyledItemDelegate>

#include <functional>
#include <vector>

// Paints the Table's reshell-only Trend column: a per-row mini-trend polyline
// (table-watch.html) computed by `ComputeSparklinePoints` from the numeric
// series the host supplies for a view row. Rows with fewer than two numeric
// samples paint only the cell background. Installed per-column by the Table
// view under the opt-in token theme.
class SparklineDelegate : public QStyledItemDelegate {
 public:
  // Returns the numeric series (oldest -> newest) for a view row.
  using SeriesProvider = std::function<std::vector<double>(int row)>;

  SparklineDelegate(SeriesProvider series_provider, QObject* parent);
  ~SparklineDelegate() override;

  // QStyledItemDelegate
  void paint(QPainter* painter,
             const QStyleOptionViewItem& option,
             const QModelIndex& index) const override;

 private:
  const SeriesProvider series_provider_;
};
