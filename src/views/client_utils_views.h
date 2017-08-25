#pragma once

#include <windows.h>

#include "base/strings/string16.h"
#include "ui/gfx/point.h"
#include "client/controls/types.h"
#include "client/client_utils.h"
#include "core/configuration_types.h"

namespace ui {
class MenuModel;
}

class ActionManager;
class OpenedView;
class MainWindow;

HMENU CreatePopupMenu(unsigned resource_id, OpenedView& view, ActionManager& action_manager);

void ShowPopupMenu(MainWindow* main_window, HMENU popup_menu,
                   const gfx::Point& point, bool right_click);

void BuildMenu(HMENU hmenu, ui::MenuModel& model, int start_position);