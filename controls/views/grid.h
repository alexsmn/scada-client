#pragma once

#include "ui/views/context_menu_controller.h"
#include "ui/views/controls/grid/grid_view.h"

class Grid : public views::GridView, private views::ContextMenuController {
 public:
  Grid(ui::GridModel& model,
       ui::HeaderModel& row_model,
       ui::HeaderModel& column_model) {
    SetModel(&model);
    SetRowModel(&row_model);
    SetColumnModel(&column_model);
  }

  void SetContextMenuHandler(ContextMenuHandler handler) {
    context_menu_handler_ = std::move(handler);
    set_context_menu_controller(this);
  }

 private:
  // views::ContextMenuController
  virtual void ShowContextMenuForView(views::View* source,
                                      const gfx::Point& point) override {
    if (context_menu_handler_)
      context_menu_handler_(point);
  }

  ContextMenuHandler context_menu_handler_;
};
