#include "controls/models/header_model.h"

namespace aui {

// HeaderModel ----------------------------------------------------------------

void HeaderModel::NotifyModelChanged() {
  for (auto& o : observers_)
    o.OnModelChanged(*this);
}

void HeaderModel::NotifySizeChanged(int index) {
  for (auto& o : observers_)
    o.OnSizeChanged(*this, index);
}

// ColumnHeaderModel ----------------------------------------------------------

void ColumnHeaderModel::SetColumnCount(int count, int column_width) {
  if (static_cast<int>(columns_.size()) == count)
    return;

  assert(count > 0);

  size_t old_count = columns_.size();

  columns_.resize(count);
  for (size_t i = old_count; i < columns_.size(); ++i) {
    columns_[i].id = i;
    columns_[i].width = column_width;
  }

  NotifyModelChanged();
}

void ColumnHeaderModel::SetSize(int index, int new_size) {
  columns_[index].width = new_size;
  NotifySizeChanged(index);
}

void ColumnHeaderModel::SetColumns(int count, const TableColumn* columns) {
  columns_.assign(columns, columns + count);
  NotifyModelChanged();
}

base::string16 ColumnHeaderModel::GetTitle(int index) const {
  return columns_[index].title;
}

}  // namespace aui
