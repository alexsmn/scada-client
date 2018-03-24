#pragma once

#include "ui/views/controls/table/table_view.h"

class Table : public views::TableView {
 public:
  Table(ui::TableModel& model, std::vector<ui::TableColumn> columns);
};
