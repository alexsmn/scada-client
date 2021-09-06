#pragma once

#include "controls/key_codes.h"
#include "gfx/point.h"

#include <SkColor.h>
#include <functional>

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

typedef std::function<void()> DoubleClickHandler;
typedef std::function<void()> SelectionChangedHandler;

typedef std::function<void(void* node, bool expanded)> TreeExpandedHandler;
typedef std::function<void(void* node, bool checked)> TreeCheckedHandler;
typedef std::function<void(void* node)> TreeDragHandler;
typedef std::function<bool(void* node)> TreeEditHandler;
typedef std::function<int(void* left, void* right)> TreeCompareHandler;
typedef std::function<void(const UiPoint& point)> ContextMenuHandler;
typedef std::function<void()> SelectionChangeHandler;

typedef std::function<bool(KeyCode key_code)> KeyPressHandler;

typedef std::function<void()> StateChangeHandler;

typedef std::function<void()> FocusHandler;

using DropAction = std::function<int()>;

#define UiColorRGB(r, g, b) SkColorSetRGB(r, g, b)

inline UiPoint ToUiPoint(POINT pt) {
  return UiPoint{pt.x, pt.y};
}
