#include "base/qt/color_qt.h"

QColor ColorToQt(SkColor color) {
  return QColor(SkColorGetR(color), SkColorGetG(color), SkColorGetB(color), SkColorGetA(color));
}

SkColor ColorFromQt(QColor qcolor) {
  return SkColorSetARGB(qcolor.alpha(), qcolor.red(), qcolor.green(), qcolor.blue());
}
