#include "aui/aui_ns_compat.h"

#include "aui/models/header_model.h"

#include "base/check.h"

namespace scada::aui {

// HeaderModel ----------------------------------------------------------------

boost::signals2::scoped_connection HeaderModel::SubscribeModelChanged(
    const ModelChangedCallback& callback) {
  return model_changed_signal_.connect(callback);
}

boost::signals2::scoped_connection HeaderModel::SubscribeSizeChanged(
    const SizeChangedCallback& callback) {
  return size_changed_signal_.connect(callback);
}

void HeaderModel::NotifyModelChanged() {
  model_changed_signal_(*this);
}

void HeaderModel::NotifySizeChanged(int index) {
  size_changed_signal_(*this, index);
}

// ColumnHeaderModel ----------------------------------------------------------

void ColumnHeaderModel::SetColumnCount(int count, int column_width) {
  if (static_cast<int>(columns_.size()) == count)
    return;

  base::Check(count > 0);

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

std::u16string ColumnHeaderModel::GetTitle(int index) const {
  return columns_[index].title;
}

}  // namespace aui
