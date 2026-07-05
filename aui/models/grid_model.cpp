#include "aui/models/grid_model.h"

#include "aui/models/grid_range.h"
#include "base/check.h"

namespace aui {

// GridModel ------------------------------------------------------------------

GridModel::GridModel() {}

GridModel::~GridModel() = default;

std::u16string GridModel::GetHint(int row, int column) {
  return std::u16string();
}

std::u16string GridModel::GetCellText(int row, int column) {
  GridCell cell;
  cell.row = row;
  cell.column = column;
  GetCell(cell);
  return cell.text;
}

bool GridModel::IsEditable(int row, int column) {
  return true;
}

bool GridModel::SetCellText(int row, int column, const std::u16string& text) {
  return false;
}

EditData GridModel::GetEditData(int row, int column) {
  return {};
}

void GridModel::HandleEditButton(int row, int column) {}

boost::signals2::scoped_connection GridModel::SubscribeModelChanged(
    const ModelChangedCallback& callback) {
  return model_changed_signal_.connect(callback);
}

boost::signals2::scoped_connection GridModel::SubscribeRowsAdded(
    const RowRangeCallback& callback) {
  return rows_added_signal_.connect(callback);
}

boost::signals2::scoped_connection GridModel::SubscribeRowsRemoved(
    const RowRangeCallback& callback) {
  return rows_removed_signal_.connect(callback);
}

boost::signals2::scoped_connection GridModel::SubscribeRangeChanged(
    const RangeChangedCallback& callback) {
  return range_changed_signal_.connect(callback);
}

void GridModel::NotifyModelChanged() {
  model_changed_signal_(*this);
}

void GridModel::NotifyRowsAdded(int first, int count) {
  rows_added_signal_(*this, first, count);
}

void GridModel::NotifyRowsRemoved(int first, int count) {
  rows_removed_signal_(*this, first, count);
}

void GridModel::NotifyRangeChanged(const GridRange& range) {
  range_changed_signal_(*this, range);
}

void GridModel::NotifyRowsChanged(int first, int count) {
  NotifyRangeChanged(GridRange::Rows(first, count));
}

}  // namespace aui
