#include "base/qt/color_qt.h"

QColor ToQColor(SkColor color) {
  return QColor(SkColorGetR(color), SkColorGetG(color), SkColorGetB(color),
                SkColorGetA(color));
}

SkColor ToSkColor(QColor qcolor) {
  return SkColorSetARGB(qcolor.alpha(), qcolor.red(), qcolor.green(),
                        qcolor.blue());
}
