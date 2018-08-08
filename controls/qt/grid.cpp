#include "grid.h"

#include <QHeaderView>

Grid::Grid(ui::GridModel& model,
           ui::HeaderModel& row_model,
           ui::HeaderModel& column_model)
    : model_{model},
      model_adapter_{model, row_model, column_model},
      item_delegate_{[&model](const QModelIndex& index) {
        return model.GetEditData(index.row(), index.column());
      }} {
  horizontalHeader()->setHighlightSections(false);
  verticalHeader()->setDefaultSectionSize(19);
  setModel(&model_adapter_);
  resizeColumnsToContents();
  setItemDelegate(&item_delegate_);
}

Grid::~Grid() {
  setModel(nullptr);
  setItemDelegate(nullptr);
}

void Grid::SetContextMenuHandler(ContextMenuHandler handler) {
  setContextMenuPolicy(Qt::CustomContextMenu);
  connect(this, &QWidget::customContextMenuRequested,
          [this, handler](const QPoint& pos) {
            handler(viewport()->mapToGlobal(pos));
          });
}
