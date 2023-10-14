#pragma once

#include <QPoint>
#include <QRect>
#include <Windows.h>

inline POINT ToPOINT(const QPoint& p) {
  return {p.x(), p.y()};
}

inline RECT ToRECT(const QRect& rect) {
  return {.left = rect.left(),
          .top = rect.top(),
          .right = rect.right(),
          .bottom = rect.bottom()};
}

inline QRect ToQRect(const RECT& rect) {
  return QRect{rect.left, rect.top, rect.right - rect.left,
               rect.bottom - rect.top};
}
