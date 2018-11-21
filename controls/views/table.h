#pragma once

#include "controls/types.h"
#include "ui/views/context_menu_controller.h"
#include "ui/views/controls/table/table_view.h"

class Table : public views::TableView, private views::ContextMenuController {
 public:
  Table(ui::TableModel& model,
        std::vector<ui::TableColumn> columns,
        bool sorting = false);

  void SetShowGrid(bool show_grid) { set_show_grid(show_grid); }

  auto GetSelectedRows() const { return selection_model().selected_indices(); }

  void SelectRow(int row, bool make_visible = true) {
    Select(row, make_visible);
  }

  void SetContextMenuHandler(ContextMenuHandler handler);

  base::Value SaveState() const;
  void RestoreState(const base::Value& data);

 private:
  // views::ContextMenuController
  virtual void ShowContextMenuForView(views::View* source,
                                      const gfx::Point& point) override;

  ContextMenuHandler context_menu_handler_;
};
