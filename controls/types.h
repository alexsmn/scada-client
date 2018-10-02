#pragma once

// TODO: Move out.
#include "core/configuration_types.h"
#include "gfx/point.h"

#include <SkColor.h>
#include <functional>
#include <set>

#if defined(UI_QT)
#include <QWidget>
#elif defined(UI_VIEWS)
#include "ui/views/view.h"
#endif

#if defined(UI_QT)
typedef QWidget UiView;
typedef QPoint UiPoint;
#elif defined(UI_VIEWS)
typedef views::View UiView;
typedef gfx::Point UiPoint;
#endif

typedef SkColor UiColor;

// TODO: Move out.
typedef std::set<scada::NodeId> NodeIdSet;

typedef std::function<void()> DoubleClickHandler;
typedef std::function<void()> SelectionChangedHandler;

typedef std::function<void(void* node, bool expanded)> TreeExpandedHandler;
typedef std::function<void(void* node, bool checked)> TreeCheckedHandler;
typedef std::function<void(void* node)> TreeDragHandler;
typedef std::function<bool(void* node)> TreeEditHandler;
typedef std::function<int(void* left, void* right)> TreeCompareHandler;
typedef std::function<void(const UiPoint& point)> ContextMenuHandler;
typedef std::function<void()> SelectionChangeHandler;

#define UiColorRGB(r, g, b) SkColorSetRGB(r, g, b)

inline UiPoint ToUiPoint(POINT pt) {
  return UiPoint{pt.x, pt.y};
}
