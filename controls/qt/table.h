#pragma once

#include "controls/qt/table_model_adapter.h"
#include "controls/types.h"
#include "ui/base/models/table_model.h"

#include <QHeaderView>
#include <QSortFilterProxyModel>
#include <QTableView>

class Table : public QTableView {
 public:
  Table(ui::TableModel& model,
        std::vector<ui::TableColumn> columns,
        bool sorting = false);
  ~Table();

  const std::vector<ui::TableColumn>& columns() const {
    return model_adapter_.columns();
  }

  void SetShowGrid(bool show_grid) { setShowGrid(show_grid); }

  void SetSelectionChangeHandler(SelectionChangeHandler handler);

  void SetContextMenuHandler(ContextMenuHandler handler);

  void SetDoubleClickHandler(DoubleClickHandler handler);

  void SetKeyPressHandler(KeyPressHandler handler);

  void SetStateChangeHandler(StateChangeHandler handler);

  int GetCurrentRow() const { return currentIndex().row(); }

  std::vector<int> GetSelectedRows() const;

  void SelectRow(int row, bool make_visible = true);

  bool editing() const { return state() == State::EditingState; };

  void OpenEditor(int row);
  void CloseEditor() {}

  QWidget* CreateParentIfNecessary() { return this; }

  base::Value SaveState() const;
  void RestoreState(const base::Value& data);

 protected:
  // QTableView
  virtual void keyPressEvent(QKeyEvent* event) override;

 private:
  QModelIndex RowToIndex(int row) const;
  int IndexToRow(const QModelIndex& index) const;

  TableModelAdapter model_adapter_;

  std::unique_ptr<QSortFilterProxyModel> proxy_model_;

  KeyPressHandler key_press_handler_;
};
