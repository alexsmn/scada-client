#include "aui/qt/header_width.h"

#include <QApplication>
#include <QFontMetrics>
#include <QStyle>

namespace scada::aui {

int HeaderTitleWidth(const QString& title) {
  if (title.isEmpty())
    return 0;
  const QFontMetrics metrics{QApplication::font("QHeaderView")};
  const QStyle* style = QApplication::style();
  const int margin = style ? style->pixelMetric(QStyle::PM_HeaderMargin) : 4;
  const int indicator =
      style ? style->pixelMetric(QStyle::PM_HeaderMarkSize) : 16;
  // The grids' header rule pads a section 6–8 px each side
  // (`QHeaderView::section{padding:…}` in grid.cpp and the parameter form),
  // which the style's own margin does not count.
  constexpr int kSheetPadding = 16;
  return metrics.horizontalAdvance(title) + 2 * margin + indicator +
         kSheetPadding;
}

}  // namespace scada::aui
