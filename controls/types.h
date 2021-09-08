#pragma once

#include "controls/key_codes.h"
#include "gfx/point.h"

#include <SkColor.h>

#if defined(UI_QT)
#include <QPoint>
#elif defined(UI_VIEWS)
#include "ui/views/view.h"
#elif defined(UI_WT)
#include <Wt/WPoint.h>
#endif

#if defined(UI_QT)
class QWidget;
typedef QWidget UiView;
typedef QPoint UiPoint;
#elif defined(UI_VIEWS)
typedef views::View UiView;
typedef gfx::Point UiPoint;
#elif defined(UI_WT)
namespace Wt {
class WWidget;
}
typedef Wt::WWidget UiView;
typedef Wt::WPoint UiPoint;
#endif

typedef SkColor UiColor;

#define UiColorRGB(r, g, b) SkColorSetRGB(r, g, b)

inline UiPoint ToUiPoint(POINT pt) {
  return UiPoint{pt.x, pt.y};
}
