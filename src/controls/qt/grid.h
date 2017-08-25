#pragma once

#include <qheaderview.h>
#include <qtableview.h>

#include "ui/base/models/grid_model.h"
#include "client/qt/grid_model_adapter.h"

class Grid : public QTableView {
 public:
  Grid(ui::GridModel& model, ui::HeaderModel& row_model, ui::HeaderModel& column_model)
      : model_adapter_(model, row_model, column_model) {
    horizontalHeader()->setHighlightSections(false);
    verticalHeader()->setDefaultSectionSize(19);
    setModel(&model_adapter_);
    resizeColumnsToContents();
  }

 private:
  GridModelAdapter model_adapter_;
};
