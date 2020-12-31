#include "components/sheet/sheet_model.h"

#include "base/stl_util.h"
#include "base/strings/sys_string_conversions.h"
#include "components/sheet/sheet_cell.h"
#include "controls/color.h"
#include "ui/base/models/grid_range.h"

// SheetColumnModel -----------------------------------------------------------

std::wstring SheetColumnModel::GetTitle(int index) const {
  assert(index >= 0);
  wchar_t ch = L'A' + static_cast<char>(index);
  return std::wstring(1, ch);
}

// SheetModel -----------------------------------------------------------------

SheetModel::SheetModel(SheetModelContext&& context)
    : SheetModelContext{std::move(context)},
      Blinker{SheetModelContext::blinker_manager_} {
  Blinker::Start();
}

SheetModel::~SheetModel() {}

void SheetModel::Load(const WindowDefinition& definition) {
  for (const auto& item : definition.items) {
    if (item.name_is("SheetCell")) {
      long row = item.GetInt("row", 0) - 1;
      long col = item.GetInt("col", 0) - 1;
      if (row < 0 || col < 0)
        continue;

      SheetCell& cell = GetCell(row, col);

      cell.SetFormula(item.GetString16("text"));

      SheetFormatBase format;

      auto align = item.GetString("align", "left");
      if (align == "right")
        format.align = DT_RIGHT;
      else if (align == "center")
        format.align = DT_CENTER;
      else
        format.align = DT_LEFT;

      auto color_string = item.GetString("color");
      if (!color_string.empty())
        format.color = aui::StringToColor(color_string);

      cell.format_ = formats().Get(format);

    } else if (item.name_is("Column")) {
      int ix = item.GetInt("ix", 0) - 1;
      if (ix < 0)
        continue;

      int width = item.GetInt("width", 0);
      if (width <= 0)
        width = 100;
      column_model().SetSize(ix, width);
    }
  }
}

void SheetModel::Save(WindowDefinition& definition) {
  for (int i = 0; i < column_count(); ++i) {
    WindowItem& item = definition.AddItem("Column");
    item.SetInt("ix", i + 1);
    item.SetInt("width", column_model().GetSize(i));
  }

  for (int i = 0; i < row_count(); i++) {
    for (int j = 0; j < column_count(); j++) {
      if (const SheetCell* cell = this->cell(i, j)) {
        WindowItem& item = definition.AddItem("SheetCell");
        // coords
        item.SetInt("row", i + 1);
        item.SetInt("col", j + 1);
        // data
        item.SetString("text", cell->formula());

        if (cell->format_) {
          // color
          if (cell->format_->color != aui::ColorCode::Transparent)
            item.SetString("color", aui::ColorToString(cell->format_->color));

          // align
          if (cell->format_->align == DT_RIGHT)
            item.SetString("align", "right");
          else if (cell->format_->align == DT_CENTER)
            item.SetString("align", "center");
        }
      }
    }
  }
}

void SheetModel::SetSizes(int row_count, int column_count) {
  assert(row_count > 0);
  assert(column_count > 0);

  if (row_count_ == row_count && column_count_ == column_count)
    return;

  std::vector<std::unique_ptr<SheetCell>> new_cells(row_count * column_count);

  int crow = (std::min)(row_count, row_count_);        // rows to copy
  int ccol = (std::min)(column_count, column_count_);  // columns to copy
  for (int i = 0; i < crow; i++) {
    for (int j = 0; j < ccol; j++)
      new_cells[i * row_count + j] = std::move(mutable_cell(i, j));
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
  const SheetCell* c = this->cell(cell.row, cell.column);
  if (!c)
    return;

  cell.text = editing_ ? c->formula() : c->text();

  if (c->format_ && c->format_->color != aui::ColorCode::Transparent)
    cell.cell_color = c->format_->color.sk_color();

  if (!editing_ && c->is_blinking() && Blinker::GetState())
    cell.cell_color = SK_ColorYELLOW;
}

SheetCell& SheetModel::GetCell(int row, int column) {
  auto& cell = mutable_cell(row, column);
  if (!cell)
    cell.reset(new SheetCell(*this, row, column));
  return *cell;
}

void SheetModel::ClearRange(const ui::GridRange& range) {
  assert(!range.empty());

  ui::GridRange update_range;

  for (int row = range.row(); row <= range.last_row(); ++row) {
    for (int column = range.column(); column <= range.last_column(); ++column) {
      auto& cell = mutable_cell(row, column);
      if (cell) {
        cell.reset();
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

  SheetCell& first_cell = **blinking_cells_.begin();

  ui::GridRange range =
      ui::GridRange::Cell(first_cell.row(), first_cell.column());

  for (auto i = std::next(blinking_cells_.begin()); i != blinking_cells_.end();
       ++i) {
    SheetCell& cell = **i;
    range.Expand(cell.row(), cell.column());
  }

  NotifyRangeChanged(range);
}

aui::Color SheetModel::GetRangeColor(const ui::GridRange& range) const {
  if (range.empty())
    return aui::ColorCode::Transparent;

  auto* cell = this->cell(range.row(), range.column());
  if (!cell || !cell->format_)
    return aui::ColorCode::Transparent;

  return cell->format_->color;
}

void SheetModel::SetRangeColor(const ui::GridRange& range, aui::Color color) {
  ui::GridRange update_range;

  for (int column = range.column(); column <= range.last_column(); ++column) {
    for (int row = range.row(); row <= range.last_row(); ++row) {
      SheetCell& cell = GetCell(row, column);

      SheetFormatBase format = cell.format_ ? *cell.format_ : SheetFormatBase();
      if (format.color != color) {
        format.color = color;
        cell.format_ = formats_.Get(format);
        update_range.Expand(cell.row(), cell.column());
      }
    }
  }

  NotifyRangeChanged(update_range);
}

bool SheetModel::SetCellText(int row, int column, const std::wstring& text) {
  GetCell(row, column).SetFormula(text);
  // TODO: Update formula row on model change notification.
  // UpdateFormulaRow();
  return true;
}

bool SheetModel::IsEditable(int row, int column) {
  return is_editing();
}
