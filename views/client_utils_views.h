#pragma once

#include "client_utils.h"
#include "controls/types.h"
#include "common/node_state.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/gfx/point.h"

#include <string>
#include <windows.h>

namespace ui {
class MenuModel;
}

HMENU CreatePopupMenu(unsigned resource_id, ui::MenuModel& context_menu_model);

void ShowPopupMenu(gfx::NativeView native_view,
                   HMENU popup_menu,
                   const gfx::Point& point,
                   bool right_click);

void BuildMenu(HMENU hmenu, ui::MenuModel& model, int start_position);
