#include "client/controls/views/table.h"

Table::Table(ui::TableModel& model, std::vector<ui::TableColumn> columns)
    : TableView(model) {
  SetColumns(columns.size(), columns.data());
}
