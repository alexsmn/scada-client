#pragma once

#include "base/observer_list.h"
#include "aui/models/table_column.h"

namespace aui {

class TableModelObserver;

struct TableCell {
  int row = 0;
  int column_id = 0;
  std::u16string text;
  Color text_color = ColorCode::Black;
  Color cell_color = ColorCode::Transparent;
  int icon_index = -1;
};

class TableModel {
 public:
  TableModel();
  virtual ~TableModel();

  TableModel(const TableModel&) = delete;
  TableModel& operator=(const TableModel&) = delete;

  virtual int GetRowCount() = 0;

  std::u16string GetCellText(int row, int column_id);

  virtual void GetCell(TableCell& cell) = 0;
  virtual std::u16string GetTooltip(int row, int column_id);

  virtual bool SetCellText(int row, int column_id, const std::u16string& text);
  virtual bool IsEditable(int row, int column_id);

  virtual void Sort(int column_id, bool ascending);
  virtual int CompareCells(int row1, int row2, int column_id);

  base::ObserverList<TableModelObserver>& observers() { return observers_; }

 protected:
  void NotifyModelChanged();
  void NotifyItemsChanged(int first, int count);
  void NotifyItemsAdding(int first, int count);
  void NotifyItemsAdded(int first, int count);
  void NotifyItemsRemoving(int first, int count);
  void NotifyItemsRemoved(int first, int count);

  base::ObserverList<TableModelObserver> observers_;
};

}  // namespace aui
