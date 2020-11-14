#include "components/cells/views/cell_view.h"

#include "base/format_time.h"
#include "common/formula_util.h"
#include "controller_factory.h"
#include "model/scada_node_ids.h"
#include "node_service/node_service.h"
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

// CellModel::Cell

CellModel::Cell::Cell(CellModel& model, int row, int column)
    : model_{model}, row_(row), column_(column) {
  value_spec_.deletion_handler = [this] { model_.ClearCell(row_, column_); };
  value_spec_.property_change_handler = [this](const PropertySet& properties) {
    model_.NotifyRangeChanged(ui::GridRange::Cell(row_, column_));
  };
}

// CellModel

CellModel::CellModel(TimedDataService& timed_data_service,
                     DialogService& dialog_service)
    : timed_data_service_{timed_data_service}, dialog_service_{dialog_service} {
  row_model_.SetSize(0, 100);
}

CellModel::~CellModel() {
  for (size_t i = 0; i != cells_.size(); ++i)
    delete cells_[i];
}

void CellModel::SetSizes(int row_count, int column_count) {
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

int CellModel::GetRowCount() {
  return row_count_;
}

void CellModel::GetCell(ui::GridCell& cell) {
  Cell* c = this->cell(cell.row, cell.column);
  if (!c)
    return;

  cell.text = c->value_spec_.GetCurrentString();
}

bool CellModel::SetCellText(int row, int column, const std::wstring& text) {
  try {
    SetCellFormula(row, column, base::SysWideToNativeMB(text));
  } catch (const std::exception& e) {
    dialog_service_.RunMessageBox(base::SysNativeMBToWide(e.what()).c_str(),
                                  L"Ошибка", MessageBoxMode::Error);
    return false;
  }

  NotifyRangeChanged(ui::GridRange::Cell(row, column));
  return true;
}

void CellModel::SetCellFormula(int row, int column, std::string_view formula) {
  Cell*& cell = this->cell(row, column);
  if (!cell)
    cell = new Cell(*this, row, column);

  cell->value_spec_.Connect(timed_data_service_, formula);

  NotifyRangeChanged(ui::GridRange::Cell(row, column));
}

bool CellModel::FindEmptyCell(int& row, int& column) {
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

void CellModel::ClearCell(int row, int column) {
  Cell*& cell = this->cell(row, column);
  if (!cell)
    return;

  delete cell;
  cell = NULL;

  NotifyRangeChanged(ui::GridRange::Cell(row, column));
}

// CellView

CellView::CellView(const ControllerContext& context)
    : Controller{context},
      grid_(new views::GridView),
      model_{timed_data_service_, dialog_service_} {
  grid_->SetModel(&model_);
  grid_->SetRowModel(&model_.row_model());
  grid_->SetColumnModel(&model_.column_model());
  grid_->set_controller(this);
  grid_->SetColumnHeaderVisible(false);
  grid_->set_context_menu_controller(this);
}

CellView::~CellView() {}

bool CellView::OnGridDrawCell(views::GridView& sender,
                              gfx::Canvas* canvas,
                              int row,
                              int col,
                              const gfx::Rect& rect) {
  canvas->FillRect(rect, ::GetSysColorBrush(COLOR_WINDOW));

  CellModel::Cell* cell = model_.cell(row, col);
  if (!cell)
    return true;

  const auto& tvq = cell->value_spec_.current();

  std::wstring lines[4];
  lines[0] = cell->value_spec_.GetTitle();
  lines[1] = cell->value_spec_.GetCurrentString();
  lines[2] = base::SysNativeMBToWide(FormatTime(tvq.source_timestamp));
  lines[3] =
      base::SysNativeMBToWide(FormatTime(cell->value_spec_.change_time()));

  gfx::Rect line_rect = rect;
  line_rect.Inset(4, 2);

  for (int i = 0; i < _countof(lines); ++i) {
    const std::wstring& line = lines[i];

    line_rect.set_height(15);

    canvas->DrawString(line, sender.font(), SK_ColorBLACK, line_rect,
                       DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);

    line_rect.set_y(line_rect.bottom());
  }

  return true;
}

void CellView::Save(WindowDefinition& definition) {
  for (int row = 0; row < model_.row_count(); ++row) {
    for (int column = 0; column < model_.column_count(); ++column) {
      if (CellModel::Cell* cell = model_.cell(row, column)) {
        WindowItem& item = definition.AddItem("Item");
        item.SetInt("row", row);
        item.SetInt("column", column);
        item.SetString("path", cell->value_spec_.formula());
      }
    }
  }
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

void CellView::ClearSelection() {
  ui::GridRange range = grid_->GetSelectionRange();
  if (range.empty())
    return;

  for (int row = range.row(); row <= range.last_row(); ++row)
    for (int column = range.column(); column <= range.last_column(); ++column)
      model_.ClearCell(row, column);

  selection().Clear();
}

void CellView::AddContainedItem(const scada::NodeId& node_id, unsigned flags) {
  int row = grid_->selected_row();
  int column = grid_->selected_column();
  if (!model_.FindEmptyCell(row, column))
    return;

  std::string path = MakeNodeIdFormula(node_id);
  model_.SetCellFormula(row, column, path);
  grid_->SelectCell(row, column, true);
}

void CellView::OnGridSelectionChanged(views::GridView& sender) {
  ui::GridRange range = grid_->GetSelectionRange();
  if (range.empty())
    selection().Clear();
  else if (range.row_count() >= 2)
    selection().SelectMultiple();
  else {
    CellModel::Cell* cell = model_.cell(range.row(), range.column());
    if (cell)
      selection().SelectTimedData(cell->value_spec_);
    else
      selection().Clear();
  }
}

views::View* CellView::Init(const WindowDefinition& definition) {
  model_.SetSizes(10, 10);

  std::deque<std::string> free_items;

  for (WindowItems::const_iterator i = definition.items.begin();
       i != definition.items.end(); ++i) {
    const WindowItem& item = *i;
    if (item.name_is("Item")) {
      int row = item.GetInt("row", -1);
      int column = item.GetInt("column", -1);
      auto path = item.GetString("path");
      if (row == -1 || column == -1)
        free_items.emplace_back(std::string{path});
      else
        model_.SetCellFormula(row, column, path);
    }
  }

  int row = -1;
  int column = -1;
  for (std::deque<std::string>::const_iterator i = free_items.begin();
       i != free_items.end(); ++i) {
    const std::string& path = *i;
    if (!model_.FindEmptyCell(row, column))
      break;
    model_.SetCellFormula(row, column, path);
  }

  return grid_->CreateParentIfNecessary();
}

void CellView::ShowContextMenuForView(views::View* source,
                                      const gfx::Point& point) {
  controller_delegate_.ShowPopupMenu(0, point, true);
}
