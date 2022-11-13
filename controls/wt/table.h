#pragma once

#include "base/values.h"
#include "controls/color.h"
#include "controls/handlers.h"

#include <Wt/WTableView.h>

namespace Wt {
class WSortFilterProxyModel;
}

namespace aui {

class TableModel;
class TableModelAdapter;
struct TableColumn;

class Table : public Wt::WTableView {
 public:
  Table(std::shared_ptr<TableModel> model,
        std::vector<TableColumn> columns,
        bool sorting = false);
  ~Table();

  const std::vector<TableColumn>& columns() const;

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

  void LoadIcons(unsigned resource_id, int width, Color mask_color);

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

}  // namespace aui
