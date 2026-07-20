
#include "aui/models/grid_range.h"

#include "base/check.h"

#include <algorithm>

namespace {

void ExpandRange(int& first, int& count, int value) {
  scada::base::Check(first >= 0);
  scada::base::Check(count >= 1);
  scada::base::Check(value >= 0);

  // If value is less than lower range, lower first and increase count.
  // If value is bigger than higher range, increase count.

  if (value < first) {
    count += first - value;
    first = value;
  } else if (value > first + count - 1) {
    count = value - first + 1;
  }
}

}  // namespace

namespace scada::aui {

GridRange GridRange::Entire() {
  GridRange range;
  range.type_ = ENTIRE;
  return range;
}

GridRange GridRange::Range(int row,
                           int column,
                           int row_count,
                           int column_count) {
  base::Check(row >= 0);
  base::Check(column >= 0);
  base::Check(row_count >= 0);
  base::Check(column_count >= 0);

  GridRange range;
  range.type_ = RANGE;
  range.row_ = row;
  range.row_count_ = row_count;
  range.column_ = column;
  range.column_count_ = column_count;

  return range;
}

GridRange GridRange::Rows(int first, int count) {
  base::Check(first >= 0);
  base::Check(count >= 1);

  GridRange range;
  range.type_ = ROWS;
  range.row_ = first;
  range.row_count_ = count;

  return range;
}

GridRange GridRange::Columns(int first, int count) {
  base::Check(first >= 0);
  base::Check(count >= 1);

  GridRange range;
  range.type_ = COLUMNS;
  range.column_ = first;
  range.column_count_ = count;

  return range;
}

bool GridRange::operator==(const GridRange& other) const {
  if (type_ != other.type_)
    return false;

  switch (type_) {
    case ENTIRE:
      return true;
    case RANGE:
      return (row_ == other.row_) && (row_count_ == other.row_count_) &&
             (column_ == other.column_) &&
             (column_count_ == other.column_count_);
    case ROWS:
      return (row_ == other.row_) && (row_count_ == other.row_count_);
    case COLUMNS:
      return (column_ == other.column_) &&
             (column_count_ == other.column_count_);
    default:
      base::NotReached();
  }
}

bool GridRange::Contains(int row, int column) const {
  base::Check(row >= 0);
  base::Check(column >= 0);

  switch (type_) {
    case RANGE:
      return (row_ <= row) && (row < row_ + row_count_) &&
             (column_ <= column) && (column < column_ + column_count_);
    case ENTIRE:
      return true;
    case ROWS:
      return (row_ <= row) && (row < row_ + row_count_);
    case COLUMNS:
      return (column_ <= column) && (column < column_ + column_count_);
    default:
      base::NotReached();
  }
}

bool GridRange::Contains(const GridRange& range) const {
  if (type_ == ENTIRE)
    return true;
  if (range.type_ == ENTIRE)
    return false;

  // Contains rows.
  if (type_ != ROWS) {
    if (range.type_ == ROWS)
      return false;
    bool contains = (row_ <= range.row_) && (range.last_row() <= last_row());
    if (!contains)
      return false;
  }

  // Contains columns.
  if (type_ != COLUMNS) {
    if (range.type_ == COLUMNS)
      return false;
    bool contains =
        (column_ <= range.column_) && (range.last_column() <= last_column());
    if (!contains)
      return false;
  }

  return true;
}

void GridRange::Expand(int row, int column) {
  base::Check(row >= 0);
  base::Check(column >= 0);

  if (empty()) {
    *this = Cell(row, column);
    return;
  }

  if (type_ == RANGE || type_ == ROWS)
    ExpandRange(row_, row_count_, row);
  if (type_ == RANGE || type_ == COLUMNS)
    ExpandRange(column_, column_count_, column);
}

void GridRange::SetLooseBounds(int row_count, int column_count) {
  base::Check(row_count >= 0);
  base::Check(column_count >= 0);

  if (type_ == ENTIRE || type_ == ROWS)
    column_count_ = std::max(1, column_count - column_);
  if (type_ == ENTIRE || type_ == COLUMNS)
    row_count_ = std::max(1, row_count - row_);
}

GridRange GridRange::Offset(int row_offset, int column_offset) const {
  if (empty() || is_entire())
    return *this;

  GridRange new_range = *this;
  if (type_ == RANGE || type_ == ROWS) {
    base::Check(row_ + row_offset >= 0);
    new_range.set_row(row_ + row_offset);
  }
  if (type_ == RANGE || type_ == COLUMNS) {
    base::Check(column_ + column_offset >= 0);
    new_range.set_column(column_ + column_offset);
  }

  return new_range;
}

}  // namespace aui
