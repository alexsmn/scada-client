#pragma once

#include <QRect>
#include <Windows.h>

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
