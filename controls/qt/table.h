#pragma once

#include "qt/table_model_adapter.h"
#include "ui/base/models/table_model.h"

#include <QHeaderView>
#include <QTableView>

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

  void SetContextMenuHandler(ContextMenuHandler handler) {
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QWidget::customContextMenuRequested,
            [this, handler](const QPoint& pos) {
              handler(viewport()->mapToGlobal(pos));
            });
  }

  QWidget* CreateParentIfNecessary() { return this; }

 private:
  TableModelAdapter model_adapter_;
};
