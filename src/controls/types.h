#pragma once

#include <functional>
#include <set>

// TODO: Move out.
#include "core/configuration_types.h"

#include "core/SkColor.h"

#if defined(UI_QT)
#include <qwidget.h>
#elif defined(UI_VIEWS)
#include "ui/views/view.h"
#endif

// TODO: Move out.
typedef std::set<scada::NodeId> NodeIdSet;

typedef std::function<void()> DoubleClickHandler;
typedef std::function<void()> SelectionChangedHandler;

typedef std::function<void(void* node, bool expanded)> TreeExpandedHandler;
typedef std::function<void(void* node, bool checked)> TreeCheckedHandler;
typedef std::function<void(void* node)> TreeDragHandler;
typedef std::function<bool(void* node)> TreeEditHandler;
typedef std::function<int(void* left, void* right)> TreeCompareHandler;

#if defined(UI_QT)
typedef QWidget UiView;
#elif defined(UI_VIEWS)
typedef views::View UiView;
#endif

typedef SkColor UiColor;

#define UiColorRGB(r, g, b) SkColorSetRGB(r, g, b)
