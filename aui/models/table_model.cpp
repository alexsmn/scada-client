#include "aui/models/table_model.h"

#include "aui/models/table_model_observer.h"

namespace aui {

TableModel::TableModel() = default;

TableModel::~TableModel() {
  DCHECK(!observers_.might_have_observers());
}

std::u16string TableModel::GetTooltip(int row, int column_id) {
  return std::u16string();
}

std::u16string TableModel::GetCellText(int row, int column_id) {
  TableCell cell;
  cell.row = row;
  cell.column_id = column_id;
  GetCell(cell);
  return cell.text;
}

bool TableModel::SetCellText(int row,
                             int column_id,
                             const std::u16string& text) {
  return false;
}

void TableModel::Sort(int column_id, bool ascending) {}

int TableModel::CompareCells(int row1, int row2, int column_id) {
  auto text1 = GetCellText(row1, column_id);
  auto text2 = GetCellText(row2, column_id);
  return text1.compare(text2);
}

void TableModel::NotifyModelChanged() {
  for (auto& o : observers_)
    o.OnModelChanged();
}

void TableModel::NotifyItemsAdding(int first, int count) {
  assert(count > 0);
  for (auto& o : observers_)
    o.OnItemsAdding(first, count);
}

void TableModel::NotifyItemsAdded(int first, int count) {
  assert(count > 0);
  for (auto& o : observers_)
    o.OnItemsAdded(first, count);
}

void TableModel::NotifyItemsRemoving(int first, int count) {
  assert(count > 0);
  for (auto& o : observers_)
    o.OnItemsRemoving(first, count);
}

void TableModel::NotifyItemsRemoved(int first, int count) {
  assert(count > 0);
  for (auto& o : observers_)
    o.OnItemsRemoved(first, count);
}

void TableModel::NotifyItemsChanged(int first, int count) {
  assert(count > 0);
  for (auto& o : observers_)
    o.OnItemsChanged(first, count);
}

bool TableModel::IsEditable(int row, int column_id) {
  return false;
}

}  // namespace aui
