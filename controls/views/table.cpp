#include "controls/views/table.h"

Table::Table(ui::TableModel& model, std::vector<ui::TableColumn> columns)
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
