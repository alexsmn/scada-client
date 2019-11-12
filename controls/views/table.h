#pragma once

#include "controls/types.h"
#include "ui/views/context_menu_controller.h"
#include "ui/views/controls/table/table_controller.h"
#include "ui/views/controls/table/table_view.h"

class Table : public views::TableView,
              private views::ContextMenuController,
              private views::TableController {
 public:
  Table(ui::TableModel& model,
        std::vector<ui::TableColumn> columns,
        bool sorting = false);

  void SetShowGrid(bool show_grid) { set_show_grid(show_grid); }

  auto GetSelectedRows() const { return selection_model().selected_indices(); }

  void SelectRow(int row, bool make_visible = true) {
    Select(row, make_visible);
  }

  void SetSortingEnabled(bool enabled) {}

  void SetSelectionChangeHandler(SelectionChangeHandler handler);

  void SetContextMenuHandler(ContextMenuHandler handler);

  void SetDoubleClickHandler(DoubleClickHandler handler);

  void SetKeyPressHandler(KeyPressHandler handler);

  void SetStateChangeHandler(StateChangeHandler handler);

  base::Value SaveState() const;
  void RestoreState(const base::Value& data);

 private:
  // views::ContextMenuController
  virtual void ShowContextMenuForView(views::View* source,
                                      const gfx::Point& point) override;

  // views::TableController
  virtual void OnSelectionChanged(views::TableView& sender) override;
  virtual bool OnDoubleClick() override;
  virtual bool OnKeyPressed(views::TableView& sender,
                            ui::KeyboardCode key_code) override;

  SelectionChangeHandler selection_change_handler_;
  ContextMenuHandler context_menu_handler_;
  DoubleClickHandler double_click_handler_;
  KeyPressHandler key_press_handler_;
};
