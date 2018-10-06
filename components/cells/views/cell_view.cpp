#include "components/cells/views/cell_view.h"

#include "base/format_time.h"
#include "common/formula_util.h"
#include "common/node_service.h"
#include "common/scada_node_ids.h"
#include "controller_factory.h"
#include "selection_model.h"
#include "services/dialog_service.h"
#include "timed_data/timed_data_service.h"
#include "ui/events/event.h"
#include "ui/events/keycodes/keyboard_codes.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/rect.h"
#include "ui/views/controls/grid/grid_view.h"
#include "views/client_utils_views.h"

#include <deque>

// CellView::Cell

CellView::Cell::Cell(CellView& view, int row, int column)
    : row_(row), column_(column) {
  value_spec_.deletion_handler = [this, &view] {
    view.ClearCell(row_, column_);
  };
  value_spec_.property_change_handler =
      [this, &view](const rt::PropertySet& properties) {
        view.grid_->SchedulePaintCell(row_, column_);
      };
}

// CellView

const WindowInfo kWindowInfo = {
    ID_CELLS_VIEW, "Cells", L"Ячейки", WIN_INS, 0, 0, 0};

REGISTER_CONTROLLER(CellView, kWindowInfo);

CellView::CellView(const ControllerContext& context)
    : Controller{context},
      row_model_(*this),
      cells_(NULL),
      grid_(new views::GridView) {
  row_model_.SetSize(0, 100);

  grid_->SetModel(this);
  grid_->SetRowModel(&row_model_);
  grid_->SetColumnModel(&column_model_);
  grid_->set_controller(this);
  grid_->SetColumnHeadersVisible(false);
  grid_->set_context_menu_controller(this);
}

CellView::~CellView() {
  for (size_t i = 0; i != cells_.size(); ++i)
    delete cells_[i];
}

void CellView::SetSizes(int row_count, int column_count) {
  DCHECK(row_count >= 0);
  DCHECK(column_count >= 0);

  std::vector<Cell*> new_cells(row_count * column_count);

  // Copy old cells to new cells.
  int copy_rows = std::min(row_count_, row_count);
  int copy_columns = std::min(column_count_, column_count);
  for (int i = 0; i < copy_rows; ++i) {
    memcpy(&new_cells[i * column_count], &cells_[i * column_count_],
           copy_columns * sizeof(cells_[0]));
  }

  // Delete cells out of new bounds.
  for (int row = 0; row < row_count_; ++row)
    for (int column = copy_columns + 1; column < column_count_; ++column)
      delete cell(row, column);
  for (int column = 0; column < column_count_; ++column)
    for (int row = copy_rows + 1; row < row_count_; ++row)
      delete cell(row, column);

  cells_.swap(new_cells);
  column_count_ = column_count;
  row_count_ = row_count;

  column_model_.SetColumnCount(column_count, 150);
  NotifyModelChanged();
}

int CellView::GetRowCount() {
  return row_count_;
}

void CellView::GetCell(ui::GridCell& cell) {
  Cell* c = this->cell(cell.row, cell.column);
  if (!c)
    return;

  cell.text = c->value_spec_.GetCurrentString();
}

bool CellView::SetCellText(int row, int column, const base::string16& text) {
  try {
    SetCellFormula(row, column, base::SysWideToNativeMB(text));
  } catch (const std::exception& e) {
    dialog_service_.RunMessageBox(base::SysNativeMBToWide(e.what()).c_str(),
                                  L"Ошибка", MessageBoxMode::Error);
    return false;
  }

  grid_->SchedulePaintCell(row, column);
  return true;
}

void CellView::SetCellFormula(int row, int column, base::StringPiece formula) {
  Cell*& cell = this->cell(row, column);
  if (!cell)
    cell = new Cell(*this, row, column);

  cell->value_spec_.Connect(timed_data_service_, formula);

  grid_->SchedulePaintCell(row, column);
}

bool CellView::OnGridDrawCell(views::GridView& sender,
                              gfx::Canvas* canvas,
                              int row,
                              int col,
                              const gfx::Rect& rect) {
  canvas->FillRect(rect, ::GetSysColorBrush(COLOR_WINDOW));

  Cell* cell = this->cell(row, col);
  if (!cell)
    return true;

  const auto& tvq = cell->value_spec_.current();

  base::string16 lines[4];
  lines[0] = cell->value_spec_.GetTitle();
  lines[1] = cell->value_spec_.GetCurrentString();
  lines[2] = base::SysNativeMBToWide(FormatTime(tvq.source_timestamp));
  lines[3] =
      base::SysNativeMBToWide(FormatTime(cell->value_spec_.change_time()));

  gfx::Rect line_rect = rect;
  line_rect.Inset(4, 2);

  for (int i = 0; i < _countof(lines); ++i) {
    const base::string16& line = lines[i];

    line_rect.set_height(15);

    canvas->DrawString(line, sender.font(), SK_ColorBLACK, line_rect,
                       DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);

    line_rect.set_y(line_rect.bottom());
  }

  return true;
}

void CellView::Save(WindowDefinition& definition) {
  for (int row = 0; row < row_count_; ++row) {
    for (int column = 0; column < column_count_; ++column) {
      if (Cell* cell = this->cell(row, column)) {
        WindowItem& item = definition.AddItem("Item");
        item.SetInt("row", row);
        item.SetInt("column", column);
        item.SetString("path", cell->value_spec_.formula());
      }
    }
  }
}

bool CellView::FindEmptyCell(int& row, int& column) {
  if (row == -1 || column == -1)
    row = 0, column = 0;
  else
    ++column;

  while (row < row_count_) {
    while (column < column_model_.GetCount()) {
      if (!cell(row, column))
        return true;
      ++column;
    }
    ++row;
  }

  return false;
}

bool CellView::OnKeyPressed(views::GridView& sender,
                            ui::KeyboardCode key_code) {
  if (grid_->editing())
    return false;

  switch (key_code) {
    case ui::VKEY_DELETE:
      ClearSelection();
      return true;
    default:
      return false;
  }
}

void CellView::ClearCell(int row, int column) {
  Cell*& cell = this->cell(row, column);
  if (!cell)
    return;

  delete cell;
  cell = NULL;

  grid_->SchedulePaintCell(row, column);
}

void CellView::ClearSelection() {
  ui::GridRange range = grid_->GetSelectionRange();
  if (range.empty())
    return;

  for (int row = range.row(); row <= range.last_row(); ++row)
    for (int column = range.column(); column <= range.last_column(); ++column)
      ClearCell(row, column);

  selection().Clear();
}

void CellView::AddContainedItem(const scada::NodeId& node_id, unsigned flags) {
  int row = grid_->selected_row();
  int column = grid_->selected_column();
  if (!FindEmptyCell(row, column))
    return;

  std::string path = MakeNodeIdFormula(node_id);
  SetCellFormula(row, column, path);
  grid_->SelectCell(row, column, true);
}

void CellView::OnGridSelectionChanged(views::GridView& sender) {
  ui::GridRange range = grid_->GetSelectionRange();
  if (range.empty())
    selection().Clear();
  else if (range.row_count() >= 2)
    selection().SelectMultiple();
  else {
    Cell* cell = this->cell(range.row(), range.column());
    if (cell)
      selection().SelectTimedData(cell->value_spec_);
    else
      selection().Clear();
  }
}

views::View* CellView::Init(const WindowDefinition& definition) {
  SetSizes(10, 10);

  std::deque<std::string> free_items;

  for (WindowItems::const_iterator i = definition.items.begin();
       i != definition.items.end(); ++i) {
    const WindowItem& item = *i;
    if (item.name_is("Item")) {
      int row = item.GetInt("row", -1);
      int column = item.GetInt("column", -1);
      auto path = item.GetString("path");
      if (row == -1 || column == -1)
        free_items.emplace_back(path.as_string());
      else
        SetCellFormula(row, column, path);
    }
  }

  int row = -1;
  int column = -1;
  for (std::deque<std::string>::const_iterator i = free_items.begin();
       i != free_items.end(); ++i) {
    const std::string& path = *i;
    if (!FindEmptyCell(row, column))
      break;
    SetCellFormula(row, column, path);
  }

  return grid_->CreateParentIfNecessary();
}

void CellView::ShowContextMenuForView(views::View* source,
                                      const gfx::Point& point) {
  controller_delegate_.ShowPopupMenu(0, point, true);
}
