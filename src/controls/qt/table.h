#pragma once

#include <qheaderview.h>
#include <qtableview.h>

#include "ui/base/models/table_model.h"
#include "qt/table_model_adapter.h"

class Table : public QTableView {
 public:
  Table(ui::TableModel& model, std::vector<ui::TableColumn> columns)
      : model_adapter_(model, std::move(columns)) {
    horizontalHeader()->setHighlightSections(false);
    verticalHeader()->setDefaultSectionSize(19);
    setModel(&model_adapter_);
    resizeColumnsToContents();
    setShowGrid(false);
  }

 private:
  TableModelAdapter model_adapter_;
};
