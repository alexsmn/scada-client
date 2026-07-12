#pragma once

#include "graph/metrix_graph.h"

#if defined(UI_QT)
#include <QColor>
#include <QRect>
#include <QSize>
#include <QWidget>

#include <functional>
#include <vector>

class QMouseEvent;
class QPaintEvent;

// The trend workspace's per-series inspector panel (see the reshell mockup
// client/docs/ui-mockups/screens/trend.html). Surfaces the selected series'
// colour, display flags (own pane / dots / stepped), configured limit bands and
// data source, and lets the operator recolour it.
//
// Reshell-only chrome: it is themed from the active design tokens and is shown
// only under the opt-in reshell theme (GetSeverityTheme() != kLegacy), like the
// rest of the trend cockpit. Custom-painted in the same spirit as the graph's
// themed legend, with a small set of clickable regions hit-tested in
// mousePressEvent.
class SeriesInspector : public QWidget {
 public:
  explicit SeriesInspector(QWidget* parent = nullptr);

  // Points the inspector at a series (null clears it). Reads the line's current
  // colour, flags, title, limits and source on each call and repaints.
  void SetLine(MetrixGraph::MetrixLine* line);

  // Invoked when the operator picks a swatch; the host applies it (e.g. via the
  // graph view's recolour path) and calls SetLine again to refresh.
  std::function<void(QColor)> on_color_chosen;

  // QWidget
  QSize sizeHint() const override;

 protected:
  void paintEvent(QPaintEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;

 private:
  MetrixGraph::MetrixLine* line_ = nullptr;

  // Swatch hit rectangles from the last paint, paired with their colour, so
  // mousePressEvent can map a click back to a colour without re-deriving the
  // palette layout.
  struct SwatchHit {
    QRect rect;
    QColor color;
  };
  std::vector<SwatchHit> swatch_hits_;
};
#endif  // UI_QT
