#include "components/sheet/sheet_model.h"

#include "base/stl_util.h"
#include "base/strings/sys_string_conversions.h"
#include "components/sheet/sheet_cell.h"
#include "ui/base/models/grid_range.h"

// SheetColumnModel -----------------------------------------------------------

base::string16 SheetColumnModel::GetTitle(int index) {
  assert(index >= 0);
  base::char16 ch = L'A' + static_cast<char>(index);
  return base::string16(1, ch);
}

// SheetModel -----------------------------------------------------------------

SheetModel::SheetModel(SheetModelContext&& context)
    : SheetModelContext{std::move(context)} {
  Blinker::Start();
}

SheetModel::~SheetModel() {
  base::STLDeleteElements(&cells_);
}

void SheetModel::SetSizes(int row_count, int column_count) {
  assert(row_count > 0);
  assert(column_count > 0);

  if (row_count_ == row_count && column_count_ == column_count)
    return;

  std::vector<SheetCell*> new_cells(row_count * column_count);

  int crow = (std::min)(row_count, row_count_);        // rows to copy
  int ccol = (std::min)(column_count, column_count_);  // columns to copy
  for (int i = 0; i < crow; i++) {
    for (int j = 0; j < ccol; j++)
      new_cells[i * row_count + j] = FindCell(i, j);
  }

  // Clear old cells.
  for (int i = 0; i < row_count_; ++i) {
    int start_col = (i < crow) ? ccol : 0;
    for (int j = start_col; j < column_count_; ++j)
      delete FindCell(i, j);
  }

  cells_.swap(new_cells);
  column_count_ = column_count;
  row_count_ = row_count;

  NotifyModelChanged();
}

void SheetModel::SetEditing(bool editing) {
  if (editing_ == editing)
    return;

  editing_ = editing;
  NotifyModelChanged();
}

int SheetModel::GetRowCount() {
  return row_count_;
}

void SheetModel::GetCell(ui::GridCell& cell) {
  SheetCell* c = FindCell(cell.row, cell.column);
  if (!c)
    return;

  cell.text = editing_ ? base::SysNativeMBToWide(c->formula()) : c->text();

  if (c->format_ && !c->format_->transparent)
    cell.cell_color = c->format_->color;

  if (!editing_ && c->is_blinking() && Blinker::GetState())
    cell.cell_color = SK_ColorYELLOW;
}

SheetCell& SheetModel::GetCell(int row, int column) {
  SheetCell*& cell = FindCell(row, column);
  if (!cell)
    cell = new SheetCell(*this, row, column);
  return *cell;
}

void SheetModel::ClearRange(const ui::GridRange& range) {
  assert(!range.empty());

  ui::GridRange update_range;

  for (int row = range.row(); row <= range.last_row(); ++row) {
    for (int column = range.column(); column <= range.last_column(); ++column) {
      SheetCell*& cell = FindCell(row, column);
      if (cell) {
        delete cell;
        cell = NULL;
        update_range.Expand(row, column);
      }
    }
  }

  NotifyRangeChanged(update_range);
}

void SheetModel::ClearCell(int row, int column) {
  ClearRange(ui::GridRange::Cell(row, column));
}

void SheetModel::OnBlink(bool state) {
  if (editing_)
    return;
  if (blinking_cells_.empty())
    return;

  SheetCell& cell = **blinking_cells_.begin();

  ui::GridRange range = ui::GridRange::Cell(cell.row(), cell.column());
  for (CellSet::iterator i = ++blinking_cells_.begin();
       i != blinking_cells_.end(); ++i) {
    SheetCell& cell = **i;
    range.Expand(cell.row(), cell.column());
  }

  NotifyRangeChanged(range);
}

void SheetModel::SetRangeColor(const ui::GridRange& range, SkColor color) {
  ui::GridRange update_range;

  for (int column = range.column(); column <= range.last_column(); ++column) {
    for (int row = range.row(); row <= range.last_row(); ++row) {
      SheetCell& cell = GetCell(row, column);

      SheetFormatBase format = cell.format_ ? *cell.format_ : SheetFormatBase();
      if (format.transparent || format.color != color) {
        format.transparent = false;
        format.color = color;
        cell.format_ = formats_.Get(format);
        update_range.Expand(cell.row(), cell.column());
      }
    }
  }

  NotifyRangeChanged(update_range);
}

bool SheetModel::SetCellText(int row, int column, const base::string16& text) {
  GetCell(row, column).SetFormula(base::SysWideToNativeMB(text));
  return true;
}
