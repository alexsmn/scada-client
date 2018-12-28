#pragma once

#include <gfx/rect.h>

#if defined(UI_QT)
#include <QRect>
#endif

#if defined(UI_QT)
inline QRect ToQRect(const gfx::Rect& rect) {
  return QRect{rect.x(), rect.y(), rect.width(), rect.height()};
}
#endif
