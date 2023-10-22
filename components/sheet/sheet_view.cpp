#include "components/sheet/sheet_view.h"

#include "aui/color.h"
#include "aui/grid.h"
#include "aui/os_exchange_data.h"
#include "base/strings/utf_string_conversions.h"
#include "client_utils.h"
#include "common/formula_util.h"
#include "common_resources.h"
#include "components/sheet/sheet_cell.h"
#include "components/sheet/sheet_model.h"
#include "controller/controller_delegate.h"
#include "item_drag_data.h"
#include "model/scada_node_ids.h"
#include "node_service/node_service.h"
#include "scada/session_service.h"
#include "controller/selection_model.h"
#include "profile/window_definition.h"

#if defined(UI_QT)
#include <QColorDialog>
#include <QLayout>
#include <QLineEdit>
#elif defined(UI_WT)
#pragma warning(push)
#pragma warning(disable : 4251 4275)
#include <wt/WContainerWidget.h>
#include <wt/WLineEdit.h>
#include <wt/WVBoxLayout.h>
#pragma warning(pop)
#endif

const int kFormulaRowHeight = 20;

// SheetController

SheetController::SheetController(const ControllerContext& context)
    : ControllerContext{context},
      model_{std::make_shared<SheetModel>(
          SheetModelContext{timed_data_service_, blinker_manager_})} {
  selection_.multiple_handler = [this] { return GetSelectedNodeIdList(); };
}

UiView* SheetController::Init(const WindowDefinition& definition) {
  model_->SetSizes(100, 100);
  model_->column_model().SetColumnCount(model_->column_count(), 65);

  model_->Load(definition);

  grid_ = new aui::Grid{
      model_, std::shared_ptr<aui::HeaderModel>{model_, &model_->row_model()},
      std::shared_ptr<aui::HeaderModel>{model_, &model_->column_model()}};

#if defined(UI_QT)
  formula_row_ = new QLineEdit;
  QObject::connect(formula_row_, &QLineEdit::editingFinished,
                   [this] { OnFormulaEdited(); });

  auto* layout = new QVBoxLayout;
  layout->setMargin(0);
  layout->setSpacing(0);
  layout->addWidget(formula_row_);
  layout->addWidget(grid_);

  contents_view_ = new QWidget;
  contents_view_->setLayout(layout);

#elif defined(UI_WT)
  formula_row_ = new Wt::WLineEdit;

  auto layout = std::make_unique<Wt::WVBoxLayout>();
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);
  layout->addWidget(std::unique_ptr<Wt::WWidget>(formula_row_));
  layout->addWidget(std::unique_ptr<Wt::WWidget>(grid_), 1);

  contents_view_ = new Wt::WContainerWidget;
  contents_view_->setLayout(std::move(layout));
#endif

  grid_->SetSelectionChangeHandler([this] { OnSelectionChanged(); });

  grid_->SetContextMenuHandler([this](const aui::Point& point) {
    controller_delegate_.ShowPopupMenu(
        model_->is_editing() ? IDR_SHEET_POPUP : 0, point, true);
  });

  command_registry_.AddCommand(
      Command{ID_EDIT}
          .set_execute_handler([this] {
            model_->SetEditing(!model_->is_editing());
            UpdateEditing();
          })
          .set_enabled_handler([this] {
            return session_service_.HasPrivilege(scada::Privilege::Configure);
          })
          .set_checked_handler([this] { return model_->is_editing(); }));

  command_registry_.AddCommand(
      Command{ID_GRAPH_COLOR}
          .set_execute_handler([this] { ChooseSelectionColor(); })
          .set_enabled_handler([this] { return model_->is_editing(); }));

  UpdateEditing();

  return contents_view_;
}

void SheetController::Save(WindowDefinition& definition) {
  model_->Save(definition);
}

void SheetController::AddContainedItem(const scada::NodeId& node_id,
                                       unsigned flags) {
  const auto& current_index = grid_->GetCurrentIndex();
  if (model_->is_editing() && current_index.column != -1 &&
      current_index.row != -1) {
    SheetCell& cell = model_->GetCell(current_index.row, current_index.column);
    cell.SetFormula(u'=' + base::UTF8ToUTF16(MakeNodeIdFormula(node_id)));
  }
}

void SheetController::UpdateEditing() {
  grid_->SetExpandAllowed(model_->is_editing());

  grid_->SetColumnHeaderVisible(model_->is_editing());
  grid_->SetRowHeaderVisible(model_->is_editing());

  UpdateFormulaRow();
}

CommandHandler* SheetController::GetCommandHandler(unsigned command_id) {
  return command_registry_.GetCommandHandler(command_id);
}

void SheetController::ChooseSelectionColor() {
#if defined(UI_QT)
  const auto& range = grid_->GetSelectionRange();
  const auto& current_color = model_->GetRangeColor(range);

  const auto& new_color = QColorDialog::getColor(current_color.qcolor(), grid_);
  if (new_color.isValid())
    model_->SetRangeColor(range, new_color);
#endif
}

void SheetController::ClearSelection() {
  const auto& range = grid_->GetSelectionRange();
  if (!range.empty())
    model_->ClearRange(range);
}

void SheetController::UpdateFormulaRow() {
#if defined(UI_QT)
  formula_row_->setVisible(model_->is_editing());
#endif

  if (!model_->is_editing())
    return;

  aui::GridRange range = grid_->GetSelectionRange();

#if defined(UI_QT)
  formula_row_->setEnabled(!range.empty());
#endif

  std::u16string text;
  if (!range.empty()) {
    const SheetCell* cell = model_->cell(range.row(), range.column());
    if (cell)
      text = cell->formula();
  }

#if defined(UI_QT)
  formula_row_->setText(QString::fromStdU16String(text));
#endif
}

void SheetController::OnFormulaEdited() {
  const auto& current_index = grid_->GetCurrentIndex();
  if (!current_index.is_valid())
    return;

#if defined(UI_QT)
  const auto& text = formula_row_->text().toStdU16String();
#elif defined(UI_WT)
  const auto& text = formula_row_->text();
#endif

  if (model_->SetCellText(current_index.row, current_index.column, text))
    grid_->RequestFocus();
}

void SheetController::OnSelectionChanged() {
  UpdateFormulaRow();

  aui::GridRange range = grid_->GetSelectionRange();
  if (range.empty()) {
    selection_.Clear();
  } else if (range.is_cell()) {
    const SheetCell* cell = model_->cell(range.row(), range.column());
    if (cell)
      selection_.SelectTimedData(cell->timed_data_);
    else
      selection_.Clear();
  } else {
    selection_.SelectMultiple();
  }
}

NodeIdSet SheetController::GetSelectedNodeIdList() {
  NodeIdSet result;

  const auto& range = grid_->GetSelectionRange();
  if (range.empty())
    return result;

  for (int row = range.row(); row <= range.last_row(); ++row) {
    for (int column = range.column(); column <= range.last_column(); ++column) {
      const SheetCell* cell = model_->cell(row, column);
      if (cell) {
        if (auto node_id = cell->timed_data().node_id(); node_id.is_null()) {
          result.emplace(std::move(node_id));
        }
      }
    }
  }

  return result;
}
