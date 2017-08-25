#include "components/sheet/sheet_view.h"

#include "base/strings/sys_string_conversions.h"
#include "base/color.h"
#include "client_utils.h"
#include "common_resources.h"
#include "views/item_drag_data.h"
#include "selection_model.h"
#include "window_definition.h"
#include "components/sheet/sheet_cell.h"
#include "components/sheet/sheet_model.h"
#include "common/scada_node_ids.h"
#include "ui/base/dragdrop/os_exchange_data.h"
#include "controls/grid.h"
#include "controller_factory.h"

#if defined(UI_QT)
#include <qlayout.h>
#include <qlineedit.h>
#elif defined(UI_VIEWS)
#include "ui/events/event.h"
#include "ui/events/keycodes/keyboard_codes.h"
#include "skia/ext/skia_utils_win.h"
#include "ui/views/controls/textfield/textfield.h"
#endif

const int kFormulaRowHeight = 20;

// SheetView::ContentsView

#if defined(UI_VIEWS)
class SheetView::ContentsView : public views::View {
 public:
  virtual void Layout() {
    views::View* formula_row = child_at(0);
    views::View* grid = child_at(1);

    int y = 0;
    if (formula_row->visible()) {
      formula_row->SetBounds(0, 0, width(), kFormulaRowHeight);
      y += kFormulaRowHeight;
    }

    grid->SetBounds(0, y, width(), std::max(0, height() - y));
  }
};
#endif

// SheetView

REGISTER_CONTROLLER(SheetView, ID_SHEET_VIEW);

SheetView::SheetView(const ControllerContext& context)
    : Controller(context),
      model_(new SheetModel(context.timed_data_service_)) {
  selection().multiple_handler_ = [this] { return GetSelectedNodeIdList(); };
}

UiView* SheetView::Init(const WindowDefinition& definition) {
  model_->SetSizes(100, 100);
  model_->column_model().SetColumnCount(model_->column_count(), 65);

  // load
  int n = 0;
  for (WindowItems::const_iterator i = definition.items.begin();
                                   i != definition.items.end(); i++)  {
    const WindowItem& item = *i;
    if (item.name_is("SheetCell")) {
      long row = item.GetInt("row", 0) - 1;
      long col = item.GetInt("col", 0) - 1;
      if (row < 0 || col < 0)
        continue;

      SheetCell& cell = model_->GetCell(row, col);

      std::string formula = item.GetString("text");
      cell.SetFormula(formula);

      SheetFormatBase format;

      std::string align = item.GetString("align", "left");
      if (align == "right")
        format.align = DT_RIGHT;
      else if (align == "center")
        format.align = DT_CENTER;
      else
        format.align = DT_LEFT;
     
      std::string color_string = item.GetString("color");
      if (!color_string.empty()) {
        format.transparent = false;
        format.color = palette::StringToColor(color_string);
      }

      cell.format_ = model_->formats().Get(format);

    } else if (item.name_is("Column")) {
      int ix = item.GetInt("ix", 0) - 1;
      if (ix < 0)
        continue;

      int width = item.GetInt("width", 0);
      if (width <= 0)
        width = 100;
      model_->column_model().SetSize(ix, width);
    }
  }

#if defined(UI_QT)
  formula_row_.reset(new QLineEdit);

  grid_.reset(new Grid(*model_, model_->row_model(), model_->column_model()));

  contents_view_.reset(new QWidget);
  contents_view_->setLayout(new QVBoxLayout);
  contents_view_->layout()->addWidget(formula_row_.get());
  contents_view_->layout()->addWidget(grid_.get());

#elif defined(UI_VIEWS)
  formula_row_.reset(new views::Textfield);
//  formula_row_->set_show_border(false);
  formula_row_->SetVisible(false);
  formula_row_->SetController(this);

  grid_.reset(new Grid(*model_, model_->row_model(), model_->column_model()));
  grid_->SetAllowDrag(true);
  grid_->set_controller(this);
  grid_->set_drop_controller(this);

  contents_view_.reset(new ContentsView);
  contents_view_->AddChildView(formula_row_.get());
  contents_view_->AddChildView(grid_->CreateParentIfNecessary());
  contents_view_->set_drop_controller(NULL);
#endif

  UpdateEditing();

  return contents_view_.get();
}

#if defined(UI_VIEWS)
void SheetView::ShowContextMenu(gfx::Point point) {
  controller_delegate_.ShowPopupMenu(IDR_SHEET_POPUP, point, true);
}

bool SheetView::OnGridEditCellText(views::GridView& sender, int row, int column,
                                   const base::string16& text) {
  model_->SetCellText(row, column, text);
  // TODO: Update formula row on model change notification.
  UpdateFormulaRow();
  return true;
}

void SheetView::OnGridGetAutocompleteList(views::GridView& sender,
                                          const base::string16& text, int& start,
                                          std::vector<base::string16>& list) {
  if (text.empty() || text[0] != '=')
    return;

  // Remove '='.
  base::string16 text2 = text.substr(1);
  int start2 = start;
  CompletePath(text2, start2, list);
  
  start = start2 + 1;
}
#endif

void SheetView::Save(WindowDefinition& definition) {
  for (int i = 0; i < model_->column_count(); ++i) {
    WindowItem& item = definition.AddItem("Column");
    item.SetInt("ix", i + 1);
    item.SetInt("width", model_->column_model().GetSize(i));
  }

  for (int i = 0; i < model_->GetRowCount(); i++) {
    for (int j = 0; j < model_->column_count(); j++) {
      SheetCell* cell = model_->FindCell(i, j);
      if (cell) {
        WindowItem& item = definition.AddItem("SheetCell");
        // coords
        item.SetInt("row", i + 1);
        item.SetInt("col", j + 1);
        // data
        item.SetString("text", cell->formula());

        if (cell->format_) {
          // color
          if (!cell->format_->transparent)
            item.SetString("color", palette::ColorToString(cell->format_->color));

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

bool SheetView::CanClose() const
{
  /*if (AtlMessageBox(m_hWnd, _T("Закрытие окна приведет к потере таблицы. Продолжить?"),
  (LPCTSTR)frame->GetTitle(), MB_YESNO|MB_ICONEXCLAMATION|MB_DEFBUTTON2) == IDNO) {
  // cancel close
  return FALSE;
  }*/
  return true;
}

void SheetView::AddContainedItem(const scada::NodeId& node_id, unsigned flags) {
#if defined(UI_VIEWS)
  if (model_->is_editing() && grid_->selected_column() != -1 && grid_->selected_row() != -1) {
    SheetCell& cell = model_->GetCell(grid_->selected_row(), grid_->selected_column());
    cell.SetFormula('=' + node_id.ToString());
  }
#endif
}

void SheetView::UpdateEditing() {
#if defined(UI_VIEWS)
  grid_->SetColumnHeadersVisible(model_->is_editing());
  grid_->SetRowHeadersVisible(model_->is_editing());
  grid_->set_allow_expand(model_->is_editing());
  UpdateFormulaRow();
  contents_view_->Layout();
#endif
}

CommandHandler* SheetView::GetCommandHandler(unsigned command_id) {
  switch (command_id) {
    case ID_EDIT:
      return this;
  }

  if (command_id >= ID_COLOR_0 && command_id < ID_COLOR_0 + palette::GetColorCount())
    return model_->is_editing() ? this : NULL;

  return __super::GetCommandHandler(command_id);
}

bool SheetView::IsCommandChecked(unsigned command_id) const
{
  switch (command_id) {
    case ID_EDIT:
      return model_->is_editing();
    default:
      return __super::IsCommandChecked(command_id);
  }
}

void SheetView::ExecuteCommand(unsigned command_id) {
  switch (command_id) {
    case ID_EDIT:
#if defined(UI_VIEWS)
      if (model_->is_editing() && !grid_->CloseEditor(true))
        break;
#endif
      model_->SetEditing(!model_->is_editing());
      UpdateEditing();
      break;
    default:
      if (command_id >= ID_COLOR_0 && command_id < ID_COLOR_0 + palette::GetColorCount()) {
        SkColor color = palette::GetColor(command_id - ID_COLOR_0);
        SetSelectionColor(color);
      } else {
        __super::ExecuteCommand(command_id);
      }
      break;
  }
}

void SheetView::ClearSelection() {
#if defined(UI_VIEWS)
  ui::GridRange range = grid_->GetSelectionRange();
  if (!range.empty())
    model_->ClearRange(range);
#endif
}

#if defined(UI_VIEWS)
bool SheetView::CanEditCell(views::GridView& sender, int row, int column) {
  return model_->is_editing();
}
#endif

void SheetView::UpdateFormulaRow() {
#if defined(UI_VIEWS)
  formula_row_->SetVisible(model_->is_editing());

  if (!model_->is_editing())
    return;

  ui::GridRange range = grid_->GetSelectionRange();

  formula_row_->SetEnabled(!range.empty());

  base::string16 text;
  if (!range.empty()) {
    SheetCell* cell = model_->FindCell(range.row(), range.column());
    if (cell)
      text = base::SysNativeMBToWide(cell->formula());
  }
  formula_row_->SetText(text);
#endif
}

#if defined(UI_VIEWS)
void SheetView::OnGridSelectionChanged(views::GridView& sender) {
  UpdateFormulaRow();

  ui::GridRange range = grid_->GetSelectionRange();
  if (range.empty()) {
    selection().Clear();
  } else if (range.is_cell()) {
    SheetCell* cell = model_->FindCell(range.row(), range.column());
    if (cell)
      selection().SelectTimedData(cell->timed_data_);
    else
      selection().Clear();
  } else {
    selection().SelectMultiple();
  }
}
#endif

void SheetView::SetSelectionColor(SkColor color) {
#if defined(UI_VIEWS)
  ui::GridRange range = grid_->GetSelectionRange();
  if (!range.empty())
    model_->SetRangeColor(range, color);
#endif
}

#if defined(UI_VIEWS)
bool SheetView::OnKeyPressed(views::GridView& sender, ui::KeyboardCode key_code) {
  switch (key_code) {
    case ui::VKEY_DELETE:
      if (!grid_->editing()) {
        ClearSelection();
        return true;
      }
      break;
  }
  
  return __super::OnKeyPressed(sender, key_code);
}

bool SheetView::OnDoubleClick() {
  SheetCell* cell = model_->FindCell(grid_->selected_row(),
                                     grid_->selected_column());  
  if (!cell)
    return false;
    
  if (cell->is_blinking()) {
    cell->timed_data().Acknowledge();
    return true;
  }

  auto node = cell->timed_data().GetNode();
  if (!node)
    return false;

  controller_delegate_.ExecuteDefaultItemCommand(node);
  return true;
}
#endif

NodeIdSet SheetView::GetSelectedNodeIdList() {
  NodeIdSet result;

#if defined(UI_VIEWS)
  ui::GridRange range = grid_->GetSelectionRange();
  if (range.empty())
    return result;

  for (int row = range.row(); row <= range.last_row(); ++row) {
    for (int column = range.column(); column <= range.last_column(); ++column) {
      SheetCell* cell = model_->FindCell(row, column);
      if (cell) {
        scada::NodeId item = cell->timed_data().trid();
        if (item != scada::NodeId())
          result.insert(item);
      }        
    }
  }
#endif

  return result;
}

#if defined(UI_VIEWS)
bool SheetView::CanDrop(const ui::OSExchangeData& data) {
  if (!model_->is_editing())
    return false;

  ItemDragData item_data;
  if (item_data.Load(data))
    return true;

  return false;
}

int SheetView::OnPerformDrop(const ui::DropTargetEvent& event) {
  grid_->SetDropRange(ui::GridRange());

  if (!model_->is_editing())
    return ui::DragDropTypes::DRAG_NONE;

  ItemDragData item_data;
  if (item_data.Load(event.data())) {
    int row = 0, col = 0;
    if (grid_->GetCellAt(event.location(), row, col)) {
      SheetCell& cell = model_->GetCell(row, col);
      std::string formula = '=' + item_data.item_id().ToString();
      cell.SetFormula(formula);
      return ui::DragDropTypes::DRAG_COPY;
    }
  }
  
  return ui::DragDropTypes::DRAG_NONE;
}

void SheetView::ContentsChanged(views::Textfield* sender,
                                const base::string16& new_contents) {
  DCHECK(sender == formula_row_.get());
  DCHECK(model_->is_editing());
  DCHECK(grid_->selected_row() != -1 && grid_->selected_column() != -1);

  model_->GetCell(grid_->selected_row(), grid_->selected_column()).
      SetFormula(base::SysWideToNativeMB(new_contents));
}
#endif
