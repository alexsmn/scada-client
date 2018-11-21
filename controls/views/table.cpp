#include "controls/views/table.h"

Table::Table(ui::TableModel& model,
             std::vector<ui::TableColumn> columns,
             bool sorting)
    : TableView(model) {
  SetColumns(columns.size(), columns.data());
  set_show_grid(false);
}

void Table::SetContextMenuHandler(ContextMenuHandler handler) {
  context_menu_handler_ = std::move(handler);
  set_context_menu_controller(this);
}

void Table::ShowContextMenuForView(views::View* source,
                                   const gfx::Point& point) {
  if (context_menu_handler_)
    context_menu_handler_(point);
}

base::Value Table::SaveState() const {
  return {};
}

void Table::RestoreState(const base::Value& data) {}
