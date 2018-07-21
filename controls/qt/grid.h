#pragma once

#include <qheaderview.h>
#include <qtableview.h>

#include "ui/base/models/grid_model.h"
#include "qt/grid_model_adapter.h"

class Grid : public QTableView {
 public:
  Grid(ui::GridModel& model, ui::HeaderModel& row_model, ui::HeaderModel& column_model)
      : model_adapter_(model, row_model, column_model) {
    horizontalHeader()->setHighlightSections(false);
    verticalHeader()->setDefaultSectionSize(19);
    setModel(&model_adapter_);
    resizeColumnsToContents();
  }

  QWidget* CreateParentIfNecessary() { return this; }

  void SetContextMenuHandler(ContextMenuHandler handler) {
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QWidget::customContextMenuRequested,
            [this, handler](const QPoint& pos) {
              handler(viewport()->mapToGlobal(pos));
            });
  }

 private:
  GridModelAdapter model_adapter_;

  ContextMenuHandler context_menu_handler_;
};
