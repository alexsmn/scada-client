#pragma once

#include "base/values.h"
#include "controls/types.h"
#include "controls/wt/table_model_adapter.h"
#include "ui/base/models/table_model.h"

#include <Wt/WSortFilterProxyModel.h>
#include <Wt/WTableView.h>

class Table : public Wt::WTableView {
 public:
  Table(std::shared_ptr<ui::TableModel> model,
        std::vector<ui::TableColumn> columns,
        bool sorting = false);
  ~Table();

  const std::vector<ui::TableColumn>& columns() const {
    return model_adapter_->columns();
  }

  void SetShowGrid(bool show_grid) {}

  void SetSelectionChangeHandler(SelectionChangeHandler handler);

  void SetContextMenuHandler(ContextMenuHandler handler);

  void SetDoubleClickHandler(DoubleClickHandler handler);

  void SetKeyPressHandler(KeyPressHandler handler);

  void SetStateChangeHandler(StateChangeHandler handler);

  int GetCurrentRow() const;

  std::vector<int> GetSelectedRows() const;

  void SelectRow(int row, bool make_visible = true);

  bool editing() const;

  void OpenEditor(int row);
  void CloseEditor() {}

  Wt::WWidget* CreateParentIfNecessary() { return this; }

  base::Value SaveState() const;
  void RestoreState(const base::Value& data);

 private:
  void keyPressEvent(const Wt::WKeyEvent& event);

  Wt::WModelIndex RowToIndex(int row) const;
  int IndexToRow(const Wt::WModelIndex& index) const;

  std::shared_ptr<TableModelAdapter> model_adapter_;

  std::shared_ptr<Wt::WSortFilterProxyModel> proxy_model_;

  KeyPressHandler key_press_handler_;
};
