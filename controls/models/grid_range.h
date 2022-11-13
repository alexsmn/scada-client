#pragma once

#include <cassert>

namespace aui {

class GridRange {
 public:
  enum Type { RANGE, ENTIRE, ROWS, COLUMNS };

  // Empty range constructor.
  GridRange()
      : type_(RANGE), row_(0), column_(0), row_count_(0), column_count_(0) {}

  Type type() const { return type_; }

  int row() const { return row_; }
  void set_row(int row) {
    assert(row >= 0);
    row_ = row;
  }
  int column() const { return column_; }
  void set_column(int column) {
    assert(column >= 0);
    column_ = column;
  }

  int row_count() const { return row_count_; }
  void set_row_count(int count) {
    assert(count >= 0);
    row_count_ = count;
  }
  int column_count() const { return column_count_; }
  void set_column_count(int count) {
    assert(count >= 0);
    column_count_ = count;
  }

  int last_row() const {
    assert(row_count_ > 0);
    return row_ + row_count_ - 1;
  }
  int last_column() const {
    assert(column_count_ > 0);
    return column_ + column_count_ - 1;
  }

  bool empty() const {
    return (type_ == RANGE) && (row_count_ == 0 || column_count_ == 0);
  }
  bool is_entire() const { return type_ == ENTIRE; }
  bool is_rows() const { return (type_ == ENTIRE) || (type_ == ROWS); }
  bool is_columns() const { return (type_ == ENTIRE) || (type_ == COLUMNS); }

  bool is_cell() const { return row_count_ == 1 && column_count_ == 1; }

  bool operator==(const GridRange& other) const;
  bool operator!=(const GridRange& other) const { return !operator==(other); }

  bool Contains(int row, int column) const;
  bool Contains(const GridRange& range) const;

  void Expand(int row, int column);

  void SetLooseBounds(int row_count, int column_count);

  GridRange Offset(int row_offset, int column_offset) const;

  static GridRange Cell(int row, int column) {
    return Range(row, column, 1, 1);
  }
  static GridRange Row(int row) { return Rows(row, 1); }
  static GridRange Column(int column) { return Columns(column, 1); }

  static GridRange Entire();
  static GridRange Range(int row, int column, int row_count, int column_count);
  static GridRange Rows(int first, int count);
  static GridRange Columns(int first, int count);

 private:
  Type type_;
  int row_;
  int column_;
  int row_count_;
  int column_count_;
};

}  // namespace aui
