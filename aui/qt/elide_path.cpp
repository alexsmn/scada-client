#include "aui/qt/elide_path.h"

#include <QFontMetrics>
#include <QStringList>

namespace scada::aui {

namespace {

bool Fits(const QString& text, const QFontMetrics& metrics, int width) {
  return metrics.horizontalAdvance(text) <= width;
}

}  // namespace

QString ElidePath(const QString& path,
                  const QString& separator,
                  const QFontMetrics& metrics,
                  int width) {
  if (Fits(path, metrics, width))
    return path;

  const QStringList segments =
      separator.isEmpty() ? QStringList{path} : path.split(separator);
  const QString& leaf = segments.back();
  const QString ellipsis{QChar{0x2026}};

  if (segments.size() > 2) {
    const QString root_and_leaf =
        segments.front() + separator + ellipsis + separator + leaf;
    if (Fits(root_and_leaf, metrics, width))
      return root_and_leaf;
  }

  if (segments.size() > 1) {
    const QString leaf_alone = ellipsis + separator + leaf;
    if (Fits(leaf_alone, metrics, width))
      return leaf_alone;
  }

  return metrics.elidedText(leaf, Qt::ElideRight, width);
}

}  // namespace scada::aui
