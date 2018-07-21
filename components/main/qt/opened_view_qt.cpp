#include "components/main/opened_view.h"

#include "components/main/action_manager.h"
#include "components/main/main_window.h"
#include "controller.h"
#include "qt/client_utils_qt.h"
#include "window_definition.h"
#include "window_info.h"

#include <QMenu>
#include <QWidget>

void OpenedView::Print() {}

void OpenedView::ShowPopupMenu(unsigned resource_id,
                               const UiPoint& point,
                               bool right_click) {
  QMenu menu;
  BuildMenu(menu, context_menu_model_);
  menu.exec(point);
}
