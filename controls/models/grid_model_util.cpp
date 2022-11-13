#include "controls/models/grid_model_util.h"

#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "controls/models/grid_model.h"
#include "controls/models/grid_range.h"

#undef StrCat

namespace aui {

// Range utils.

void SetFirstRow(GridRange& range, int row) {
  assert(row >= 0);
  assert(row <= range.last_row());

  int last_row = range.last_row();
  range.set_row(row);
  range.set_row_count(last_row - row + 1);
}

void SetFirstColumn(GridRange& range, int column) {
  assert(column >= 0);
  assert(column <= range.last_column());

  int last_column = range.last_column();
  range.set_column(column);
  range.set_column_count(last_column - column + 1);
}

void SetLastRow(GridRange& range, int last_row) {
  assert(last_row >= 0);
  assert(last_row >= range.row());

  range.set_row_count(last_row - range.row() + 1);
}

void SetLastColumn(GridRange& range, int last_column) {
  assert(last_column >= 0);
  assert(last_column >= range.row());

  range.set_column_count(last_column - range.column() + 1);
}

int Digit(base::char16 ch) {
  int res = static_cast<int>(ch - L'0');
  if (res < 0 || res > 9)
    return -1;
  else
    return res;
}

bool GetBaseNum(base::string16& base, int& num) {
  if (base.empty())
    return false;
  if (Digit(base[base.length() - 1]) == -1)
    return false;

  num = 0;
  int exp = 1;
  while (!base.empty()) {
    int dig = Digit(base[base.length() - 1]);
    if (dig == -1)
      break;
    num += dig * exp;
    exp *= 10;
    base.resize(base.length() - 1);
  }
  return exp != 1;
}

base::string16 SetBaseNum(const base::char16* base, int num) {
  return base::StrCat({base, base::NumberToString16(num)});
}

class Expander {
 public:
  explicit Expander(GridModel& model) : model_(model) {}

  void Expand(bool columns,
              int source_row,
              int first_cell,
              int cell_count,
              int dest_first_row,
              int dest_row_count) {
    assert(source_row >= 0);
    assert(first_cell >= 0);
    assert(cell_count > 0);
    assert(dest_first_row >= 0);
    assert(dest_row_count > 0);
    // Source row isn't contained in range of destination rows.
    assert((dest_first_row > source_row) ||
           (source_row > dest_first_row + dest_row_count - 1));

    columns_ = columns;
    first_cell_ = first_cell;

    source_cells_.resize(cell_count);
    for (int i = 0; i < cell_count; ++i) {
      SourceCell& source = source_cells_[i];
      base::string16 text =
          columns ? model_.GetCellText(first_cell + i, source_row)
                  : model_.GetCellText(source_row, first_cell + i);
      source.base_text = text;
      source.base_index = 0;
      source.autoincrement = GetBaseNum(source.base_text, source.base_index);
    }

    bool dest_direction_down = dest_first_row > source_row;
    int dest_last_row = dest_first_row + dest_row_count - 1;
    for (int i = 0; i < dest_row_count; ++i) {
      int dest_row =
          dest_direction_down ? dest_first_row + i : dest_last_row - i;
      ExpandRow(dest_row, dest_row - source_row);
    }
  }

 private:
  struct SourceCell {
    base::string16 base_text;
    bool autoincrement;
    int base_index;
  };

  void ExpandRow(int row, int offset) {
    assert(row >= 0);
    for (int i = 0; i < static_cast<int>(source_cells_.size()); ++i)
      ExpandCell(row, first_cell_ + i, source_cells_[i], offset);
  }

  void ExpandCell(int row, int column, const SourceCell& source, int offset) {
    base::string16 new_text;
    if (source.autoincrement) {
      new_text =
          SetBaseNum(source.base_text.c_str(), source.base_index + offset);
    } else {
      new_text = source.base_text;
    }

    if (columns_)
      model_.SetCellText(column, row, new_text);
    else
      model_.SetCellText(row, column, new_text);
  }

  GridModel& model_;

  bool columns_ = false;

  int first_cell_ = 0;

  std::vector<SourceCell> source_cells_;
};

void ExpandGridRange(GridModel& model,
                     const GridRange& range,
                     const GridRange& new_range,
                     bool increment) {
  assert(!range.empty());
  assert(new_range.Contains(range));

  // Calculate range to fill with values. It's |new_range| excluding |range|.
  GridRange fill_range = new_range;
  if (fill_range.column() < range.column())
    SetLastColumn(fill_range, range.column() - 1);
  else if (fill_range.last_column() > range.last_column())
    SetFirstColumn(fill_range, range.last_column() + 1);
  else if (fill_range.row() < range.row())
    SetLastRow(fill_range, range.row() - 1);
  else if (fill_range.last_row() > range.last_row())
    SetFirstRow(fill_range, range.last_row() + 1);

  if (!increment) {
    // copy with CTRL
    base::string16 text = model.GetCellText(range.row(), range.column());
    for (int row = fill_range.row(); row <= fill_range.last_row(); row++) {
      for (int col = fill_range.column(); col <= fill_range.last_column();
           col++)
        model.SetCellText(row, col, text);
    }

  } else {
    // progress
    Expander expander(model);
    bool columns = (new_range.column() < range.column()) ||
                   (new_range.last_column() > range.last_column());
    if (columns) {
      expander.Expand(true, range.column(), range.row(), range.row_count(),
                      fill_range.column(), fill_range.column_count());
    } else {
      expander.Expand(false, range.row(), range.column(), range.column_count(),
                      fill_range.row(), fill_range.row_count());
    }
  }
}

}  // namespace aui
