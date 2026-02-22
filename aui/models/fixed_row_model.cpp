#include "aui/models/fixed_row_model.h"

#include "base/format.h"

namespace aui {

// FixedRowModel --------------------------------------------------------------

FixedRowModel::FixedRowModel(Delegate& delegate)
    : delegate_(delegate) {
  SetFixedSize(true);
}

int FixedRowModel::GetCount() const {
  return delegate_.GetRowCount();
}

int FixedRowModel::GetSize(int index) const {
  return row_height_;
}

base::string16 FixedRowModel::GetTitle(int index) const {
  return delegate_.GetRowTitle(index);
}

// FixedRowModel::Delegate ----------------------------------------------------

base::string16 FixedRowModel::Delegate::GetRowTitle(int index) {
  return WideFormat(index + 1);
}

}  // namespace aui
