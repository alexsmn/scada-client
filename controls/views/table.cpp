#include "controls/views/table.h"

Table::Table(std::shared_ptr<ui::TableModel> model,
             std::vector<ui::TableColumn> columns,
             bool sorting)
    : TableView(*model), model_{model} {
  SetColumns(columns.size(), columns.data());
  set_show_grid(false);
  set_controller(this);
}

void Table::SetSelectionChangeHandler(SelectionChangeHandler handler) {
  selection_change_handler_ = std::move(handler);
}

void Table::SetContextMenuHandler(ContextMenuHandler handler) {
  context_menu_handler_ = std::move(handler);
  set_context_menu_controller(this);
}

void Table::SetDoubleClickHandler(DoubleClickHandler handler) {
  double_click_handler_ = std::move(handler);
}

void Table::ShowContextMenuForView(views::View* source,
                                   const gfx::Point& point) {
  if (context_menu_handler_)
    context_menu_handler_(point);
}

void Table::SetKeyPressHandler(KeyPressHandler handler) {
  key_press_handler_ = std::move(handler);
}

base::Value Table::SaveState() const {
  return {};
}

void Table::RestoreState(const base::Value& data) {}

void Table::OnSelectionChanged(views::TableView& sender) {
  if (selection_change_handler_)
    selection_change_handler_();
}

bool Table::OnDoubleClick() {
  if (!double_click_handler_)
    return false;

  double_click_handler_();
  return true;
}

bool Table::OnKeyPressed(views::TableView& sender, ui::KeyboardCode key_code) {
  return key_press_handler_ &&
         key_press_handler_(static_cast<KeyCode>(key_code));
}

void Table::SetStateChangeHandler(StateChangeHandler handler) {}
