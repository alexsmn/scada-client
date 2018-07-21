#pragma once

#include "controls/types.h"
#include "ui/views/context_menu_controller.h"
#include "ui/views/controls/table/table_view.h"

class Table : public views::TableView, private views::ContextMenuController {
 public:
  Table(ui::TableModel& model, std::vector<ui::TableColumn> columns);

  void SetContextMenuHandler(ContextMenuHandler handler);

 private:
  // views::ContextMenuController
  virtual void ShowContextMenuForView(views::View* source,
                                      const gfx::Point& point) override;

  ContextMenuHandler context_menu_handler_;
};
