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
    setSelectionBehavior(SelectRows);
  }

  const std::vector<ui::TableColumn>& columns() const {
    return model_adapter_.columns();
  }

  void SetShowGrid(bool show_grid) { setShowGrid(show_grid); }

  void SetContextMenuHandler(ContextMenuHandler handler) {
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QWidget::customContextMenuRequested,
            [this, handler](const QPoint& pos) {
              handler(viewport()->mapToGlobal(pos));
            });
  }

  int GetCurrentRow() const { return currentIndex().row(); }

  std::vector<int> GetSelectedRows() const {
    std::vector<int> rows;
    if (selectionModel()) {
      auto indexes = selectionModel()->selectedRows();
      rows.reserve(indexes.size());
      for (const auto& index : indexes)
        rows.emplace_back(index.row());
    }
    return rows;
  }

  void SelectRow(int row, bool make_visible = true) { selectRow(row); }

  void OpenEditor(int row) {
    openPersistentEditor(model()->index(row, 0, rootIndex()));
  }

  void CloseEditor() { closePersistentEditor(currentIndex()); }

  QWidget* CreateParentIfNecessary() { return this; }

 private:
  TableModelAdapter model_adapter_;
};
