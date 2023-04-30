#include "controls/models/grid_model.h"

#include "controls/models/grid_range.h"

namespace aui {

// GridModel ------------------------------------------------------------------

GridModel::GridModel() {}

GridModel::~GridModel() {
  DCHECK(!observers_.might_have_observers());
}

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

void GridModel::NotifyModelChanged() {
  for (auto& o : observers_)
    o.OnGridModelChanged(*this);
}

void GridModel::NotifyRowsAdded(int first, int count) {
  for (auto& o : observers_)
    o.OnGridRowsAdded(*this, first, count);
}

void GridModel::NotifyRowsRemoved(int first, int count) {
  for (auto& o : observers_)
    o.OnGridRowsRemoved(*this, first, count);
}

void GridModel::NotifyRangeChanged(const GridRange& range) {
  for (auto& o : observers_)
    o.OnGridRangeChanged(*this, range);
}

void GridModel::NotifyRowsChanged(int first, int count) {
  NotifyRangeChanged(GridRange::Rows(first, count));
}

}  // namespace aui
