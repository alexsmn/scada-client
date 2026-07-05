#include "aui/models/table_model.h"

#include "base/check.h"

namespace aui {

TableModel::TableModel() = default;

TableModel::~TableModel() = default;

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

boost::signals2::scoped_connection TableModel::SubscribeModelChanged(
    const ModelChangedCallback& callback) {
  return model_changed_signal_.connect(callback);
}

boost::signals2::scoped_connection TableModel::SubscribeItemsChanged(
    const ItemRangeCallback& callback) {
  return items_changed_signal_.connect(callback);
}

boost::signals2::scoped_connection TableModel::SubscribeItemsAdding(
    const ItemRangeCallback& callback) {
  return items_adding_signal_.connect(callback);
}

boost::signals2::scoped_connection TableModel::SubscribeItemsAdded(
    const ItemRangeCallback& callback) {
  return items_added_signal_.connect(callback);
}

boost::signals2::scoped_connection TableModel::SubscribeItemsRemoving(
    const ItemRangeCallback& callback) {
  return items_removing_signal_.connect(callback);
}

boost::signals2::scoped_connection TableModel::SubscribeItemsRemoved(
    const ItemRangeCallback& callback) {
  return items_removed_signal_.connect(callback);
}

void TableModel::NotifyModelChanged() {
  model_changed_signal_();
}

void TableModel::NotifyItemsAdding(int first, int count) {
  base::Check(count > 0);
  items_adding_signal_(first, count);
}

void TableModel::NotifyItemsAdded(int first, int count) {
  base::Check(count > 0);
  items_added_signal_(first, count);
}

void TableModel::NotifyItemsRemoving(int first, int count) {
  base::Check(count > 0);
  items_removing_signal_(first, count);
}

void TableModel::NotifyItemsRemoved(int first, int count) {
  base::Check(count > 0);
  items_removed_signal_(first, count);
}

void TableModel::NotifyItemsChanged(int first, int count) {
  base::Check(count > 0);
  items_changed_signal_(first, count);
}

bool TableModel::IsEditable(int row, int column_id) {
  return false;
}

}  // namespace aui
