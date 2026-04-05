#pragma once

#include "aui/key_codes.h"
#include "aui/point.h"

#include <functional>

typedef std::function<void()> DoubleClickHandler;
typedef std::function<void()> SelectionChangedHandler;

typedef std::function<void(void* node, bool expanded)> TreeExpandedHandler;
typedef std::function<void(void* node, bool checked)> TreeCheckedHandler;
typedef std::function<void(void* node)> TreeDragHandler;
typedef std::function<bool(void* node)> TreeEditHandler;
typedef std::function<int(void* left, void* right)> TreeCompareHandler;
typedef std::function<void(const aui::Point& point)> ContextMenuHandler;
typedef std::function<void()> SelectionChangeHandler;

typedef std::function<bool(aui::KeyCode key_code)> KeyPressHandler;

typedef std::function<void()> StateChangeHandler;

typedef std::function<void()> FocusHandler;

using DragData = std::unordered_map<std::string, std::vector<char>>;
using DragHandler = std::function<DragData(const std::vector<void*>& nodes)>;

using DropAction = std::function<int()>;
using DropHandler = std::function<
    DropAction(int drop_action, const DragData& drag_data, void* target_node)>;
