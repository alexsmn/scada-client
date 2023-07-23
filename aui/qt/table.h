#pragma once

#include "base/values.h"
#include "aui/color.h"
#include "aui/handlers.h"

#include <QTableView>

class QSortFilterProxyModel;

namespace aui {

class TableModel;
class TableModelAdapter;
struct TableColumn;

class Table : public QTableView {
 public:
  Table(std::shared_ptr<TableModel> model,
        std::vector<TableColumn> columns,
        bool sorting = false);
  ~Table();

  const std::vector<TableColumn>& columns() const;

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

  void LoadIcons(unsigned resource_id, int width, Color mask_color);

  base::Value SaveState() const;
  void RestoreState(const base::Value& data);

  void CopyToClipbard();

 protected:
  // QTableView
  virtual void keyPressEvent(QKeyEvent* event) override;

 private:
  QModelIndex RowToIndex(int row) const;
  int IndexToRow(const QModelIndex& index) const;

  std::unique_ptr<TableModelAdapter> model_adapter_;

  std::unique_ptr<QSortFilterProxyModel> proxy_model_;

  KeyPressHandler key_press_handler_;
};

}  // namespace aui
