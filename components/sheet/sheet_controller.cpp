#include "components/sheet/sheet_controller.h"

#include "base/strings/sys_string_conversions.h"
#include "client_utils.h"
#include "common/formula_util.h"
#include "common/node_service.h"
#include "common_resources.h"
#include "components/sheet/sheet_cell.h"
#include "components/sheet/sheet_model.h"
#include "controller_factory.h"
#include "controls/color.h"
#include "controls/grid.h"
#include "model/scada_node_ids.h"
#include "selection_model.h"
#include "ui/base/dragdrop/os_exchange_data.h"
#include "views/item_drag_data.h"
#include "window_definition.h"

#if defined(UI_QT)
#include <QLayout>
#include <QLineEdit>
#elif defined(UI_VIEWS)
#include "skia/ext/skia_utils_win.h"
#include "ui/events/event.h"
#include "ui/events/keycodes/keyboard_codes.h"
#include "ui/views/controls/textfield/textfield.h"
#endif

const int kFormulaRowHeight = 20;

const WindowInfo kWindowInfo = {
    ID_SHEET_VIEW,  "CusTable", L"Пользовательская таблица", WIN_INS, 0, 0,
    IDR_SHEET_POPUP};

REGISTER_CONTROLLER(SheetController, kWindowInfo);

// SheetController::ContentsView

#if defined(UI_VIEWS)
class SheetController::ContentsView : public views::View {
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

// SheetController

SheetController::SheetController(const ControllerContext& context)
    : Controller{context},
      model_{std::make_unique<SheetModel>(
          SheetModelContext{timed_data_service_})} {
  selection().multiple_handler = [this] { return GetSelectedNodeIdList(); };
}

UiView* SheetController::Init(const WindowDefinition& definition) {
  model_->SetSizes(100, 100);
  model_->column_model().SetColumnCount(model_->column_count(), 65);

  // load
  model_->Load(definition);

#if defined(UI_QT)
  formula_row_.reset(new QLineEdit);

  grid_.reset(new Grid(*model_, model_->row_model(), model_->column_model()));

  auto* layout = new QVBoxLayout;
  layout->setSpacing(0);
  layout->addWidget(formula_row_.get());
  layout->addWidget(grid_.get());

  contents_view_.reset(new QWidget);
  contents_view_->setLayout(layout);

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

  grid_->SetContextMenuHandler([this](const UiPoint& point) {
    controller_delegate_.ShowPopupMenu(IDR_SHEET_POPUP, point, true);
  });

  UpdateEditing();

  return contents_view_.get();
}

#if defined(UI_VIEWS)
void SheetController::OnGridGetAutocompleteList(
    views::GridView& sender,
    const base::string16& text,
    int& start,
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

void SheetController::Save(WindowDefinition& definition) {
  model_->Save(definition);
}

bool SheetController::CanClose() const {
  /*if (AtlMessageBox(m_hWnd, _T("Закрытие окна приведет к потере таблицы.
  Продолжить?"), (LPCTSTR)frame->GetTitle(),
  MB_YESNO|MB_ICONEXCLAMATION|MB_DEFBUTTON2) == IDNO) {
  // cancel close
  return FALSE;
  }*/
  return true;
}

void SheetController::AddContainedItem(const scada::NodeId& node_id,
                                       unsigned flags) {
#if defined(UI_VIEWS)
  if (model_->is_editing() && grid_->selected_column() != -1 &&
      grid_->selected_row() != -1) {
    SheetCell& cell =
        model_->GetCell(grid_->selected_row(), grid_->selected_column());
    cell.SetFormula(L'=' + base::SysNativeMBToWide(MakeNodeIdFormula(node_id)));
  }
#endif
}

void SheetController::UpdateEditing() {
  grid_->SetExpandAllowed(model_->is_editing());

  grid_->SetColumnHeaderVisible(model_->is_editing());
  grid_->SetRowHeaderVisible(model_->is_editing());

  UpdateFormulaRow();

#if defined(UI_VIEWS)
  contents_view_->Layout();
#endif
}

CommandHandler* SheetController::GetCommandHandler(unsigned command_id) {
  switch (command_id) {
    case ID_EDIT:
      return this;
  }

  if (command_id >= ID_COLOR_0 &&
      command_id < ID_COLOR_0 + aui::GetColorCount())
    return model_->is_editing() ? this : NULL;

  return Controller::GetCommandHandler(command_id);
}

bool SheetController::IsCommandChecked(unsigned command_id) const {
  switch (command_id) {
    case ID_EDIT:
      return model_->is_editing();
    default:
      return __super::IsCommandChecked(command_id);
  }
}

void SheetController::ExecuteCommand(unsigned command_id) {
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
      if (command_id >= ID_COLOR_0 &&
          command_id < ID_COLOR_0 + aui::GetColorCount()) {
        auto color = aui::GetColor(command_id - ID_COLOR_0);
        SetSelectionColor(color.sk_color());
      } else {
        __super::ExecuteCommand(command_id);
      }
      break;
  }
}

void SheetController::ClearSelection() {
#if defined(UI_VIEWS)
  ui::GridRange range = grid_->GetSelectionRange();
  if (!range.empty())
    model_->ClearRange(range);
#endif
}

void SheetController::UpdateFormulaRow() {
#if defined(UI_QT)
  formula_row_->setVisible(model_->is_editing());

#elif defined(UI_VIEWS)
  formula_row_->SetVisible(model_->is_editing());

  if (!model_->is_editing())
    return;

  ui::GridRange range = grid_->GetSelectionRange();

  formula_row_->SetEnabled(!range.empty());

  base::string16 text;
  if (!range.empty()) {
    const SheetCell* cell = model_->cell(range.row(), range.column());
    if (cell)
      text = cell->formula();
  }
  formula_row_->SetText(text);
#endif
}

#if defined(UI_VIEWS)
void SheetController::OnGridSelectionChanged(views::GridView& sender) {
  UpdateFormulaRow();

  ui::GridRange range = grid_->GetSelectionRange();
  if (range.empty()) {
    selection().Clear();
  } else if (range.is_cell()) {
    const SheetCell* cell = model_->cell(range.row(), range.column());
    if (cell)
      selection().SelectTimedData(cell->timed_data_);
    else
      selection().Clear();
  } else {
    selection().SelectMultiple();
  }
}
#endif

void SheetController::SetSelectionColor(SkColor color) {
#if defined(UI_VIEWS)
  ui::GridRange range = grid_->GetSelectionRange();
  if (!range.empty())
    model_->SetRangeColor(range, color);
#endif
}

#if defined(UI_VIEWS)
bool SheetController::OnKeyPressed(views::GridView& sender,
                                   ui::KeyboardCode key_code) {
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

bool SheetController::OnDoubleClick() {
  const SheetCell* cell =
      model_->cell(grid_->selected_row(), grid_->selected_column());
  if (!cell)
    return false;

  if (cell->is_blinking()) {
    cell->timed_data().Acknowledge();
    return true;
  }

  auto node = cell->timed_data().GetNode();
  if (!node)
    return false;

  controller_delegate_.ExecuteDefaultNodeCommand(node);

  return true;
}
#endif

NodeIdSet SheetController::GetSelectedNodeIdList() {
  NodeIdSet result;

#if defined(UI_VIEWS)
  ui::GridRange range = grid_->GetSelectionRange();
  if (range.empty())
    return result;

  for (int row = range.row(); row <= range.last_row(); ++row) {
    for (int column = range.column(); column <= range.last_column(); ++column) {
      const SheetCell* cell = model_->cell(row, column);
      if (cell) {
        auto node_id = cell->timed_data().GetNode().node_id();
        if (node_id.is_null())
          result.insert(node_id);
      }
    }
  }
#endif

  return result;
}

#if defined(UI_VIEWS)
bool SheetController::CanDrop(const ui::OSExchangeData& data) {
  if (!model_->is_editing())
    return false;

  ItemDragData item_data;
  if (item_data.Load(data))
    return true;

  return false;
}

int SheetController::OnPerformDrop(const ui::DropTargetEvent& event) {
  grid_->SetDropRange(ui::GridRange());

  if (!model_->is_editing())
    return ui::DragDropTypes::DRAG_NONE;

  ItemDragData item_data;
  if (item_data.Load(event.data())) {
    int row = 0, col = 0;
    if (grid_->GetCellAt(event.location(), row, col)) {
      SheetCell& cell = model_->GetCell(row, col);
      cell.SetFormula(L'=' + base::SysNativeMBToWide(
                                 MakeNodeIdFormula(item_data.item_id())));
      return ui::DragDropTypes::DRAG_COPY;
    }
  }

  return ui::DragDropTypes::DRAG_NONE;
}

void SheetController::ContentsChanged(views::Textfield* sender,
                                      const base::string16& new_contents) {
  DCHECK(sender == formula_row_.get());
  DCHECK(model_->is_editing());
  DCHECK(grid_->selected_row() != -1 && grid_->selected_column() != -1);

  model_->GetCell(grid_->selected_row(), grid_->selected_column())
      .SetFormula(new_contents);
}
#endif
