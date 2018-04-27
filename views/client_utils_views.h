#pragma once

#include <windows.h>

#include "base/strings/string16.h"
#include "client_utils.h"
#include "controls/types.h"
#include "core/configuration_types.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/gfx/point.h"

namespace ui {
class MenuModel;
}

class ActionManager;
class OpenedView;
class MainWindowViews;

HMENU CreatePopupMenu(unsigned resource_id,
                      MainWindowViews& main_window,
                      ActionManager& action_manager);

void ShowPopupMenu(gfx::NativeView native_view,
                   HMENU popup_menu,
                   const gfx::Point& point,
                   bool right_click);

void BuildMenu(HMENU hmenu, ui::MenuModel& model, int start_position);
