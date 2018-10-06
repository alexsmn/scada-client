#pragma once

#include "controls/types.h"
#include "qt/table_model_adapter.h"
#include "ui/base/models/table_model.h"

#include <QHeaderView>
#include <QTableView>

class Table : public QTableView {
 public:
  Table(ui::TableModel& model, std::vector<ui::TableColumn> columns);

  const std::vector<ui::TableColumn>& columns() const {
    return model_adapter_.columns();
  }

  void SetShowGrid(bool show_grid) { setShowGrid(show_grid); }

  void SetContextMenuHandler(ContextMenuHandler handler);

  int GetCurrentRow() const { return currentIndex().row(); }

  std::vector<int> GetSelectedRows() const;

  void SelectRow(int row, bool make_visible = true) { selectRow(row); }

  void OpenEditor(int row) { edit(model()->index(row, 0, rootIndex())); }

  void CloseEditor() {}

  QWidget* CreateParentIfNecessary() { return this; }

  base::Value SaveState() const;
  void RestoreState(const base::Value& data);

 private:
  TableModelAdapter model_adapter_;
};
