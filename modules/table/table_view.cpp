#include "modules/table/table_view.h"

#include "aui/dialog_service.h"
#include "aui/severity_colors.h"
#include "aui/table.h"
#include "aui/translation.h"
#include "common/formula_util.h"
#include "controller/controller_delegate.h"
#include "controller/selection_model.h"
#include "model/data_items_node_ids.h"
#include "model/scada_node_ids.h"
#include "modules/table/table_model.h"
#include "modules/table/table_row.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"
#include "profile/profile.h"
#include "resources/common_resources.h"
#include "ui/common/client_utils.h"

#if defined(UI_QT)
#include "modules/table/qt/sparkline_delegate.h"
#include "modules/table/qt/table_toolbar.h"
#include "modules/table/sparkline.h"

#include <QVBoxLayout>
#include <QWidget>
#endif

// TableView

TableView::TableView(const ControllerContext& context)
    : ControllerContext{context},
      model_{std::make_shared<TableModel>(
          TableModelContext{timed_data_service_, node_event_provider_, profile_,
                            dialog_service_, blinker_manager_})} {
  model_->item_changed_ = [this](const scada::NodeId& item_id, bool added) {
    NotifyContainedItemChanged(item_id, added);
  };

  std::vector<scada::aui::TableColumn> columns = {
      {TableModel::COLUMN_TITLE, Translate("Name"), 150,
       scada::aui::TableColumn::LEFT},
      {TableModel::COLUMN_VALUE, Translate("Value"), 100,
       scada::aui::TableColumn::RIGHT,
       scada::aui::TableColumn::DataType::General,
       /*monospace=*/true},
  };

  // Reshell-only quality mark and per-row mini-trend columns, placed next to
  // the value exactly as in table-watch.html. Gated on the opt-in token theme
  // so the legacy grid is unchanged (the good/uncertain/bad tokens only exist
  // under the token themes; `QualityColor` returns nothing under kLegacy
  // anyway).
  [[maybe_unused]] int sparkline_column = -1;
  if (scada::aui::GetSeverityTheme() != scada::aui::SeverityTheme::kLegacy) {
    columns.push_back({TableModel::COLUMN_QUALITY, Translate("Quality"), 110,
                       scada::aui::TableColumn::LEFT});
    sparkline_column = static_cast<int>(columns.size());
    columns.push_back({TableModel::COLUMN_SPARKLINE, Translate("Trend"), 120,
                       scada::aui::TableColumn::LEFT});
  }

  columns.insert(
      columns.end(),
      {{TableModel::COLUMN_SOURCE_TIMESTAMP, Translate("Source Timestamp"), 170,
        scada::aui::TableColumn::LEFT,
        scada::aui::TableColumn::DataType::DateTime},
       {TableModel::COLUMN_SERVER_TIMESTAMP, Translate("Server Timestamp"), 170,
        scada::aui::TableColumn::LEFT,
        scada::aui::TableColumn::DataType::DateTime},
       {TableModel::COLUMN_CHANGE_TIME, Translate("Change Time"), 170,
        scada::aui::TableColumn::LEFT,
        scada::aui::TableColumn::DataType::DateTime},
       {TableModel::COLUMN_EVENT, Translate("Event"), 200,
        scada::aui::TableColumn::LEFT}});

  // cppcheck-suppress noCopyConstructor
  // cppcheck-suppress noOperatorEq
  view_ = new scada::aui::Table{model_, std::move(columns)};

#if defined(UI_QT)
  if (sparkline_column != -1) {
    // The Trend cells are painted, not textual: a per-row mini-trend from the
    // row's trailing history window (see TableRow::SetFormula).
    view_->setItemDelegateForColumn(
        sparkline_column,
        new SparklineDelegate{
            [this](int row) {
              const TableRow* table_row = model_->GetRow(row);
              return table_row ? NumericSeries(table_row->timed_data().values())
                               : std::vector<double>{};
            },
            view_});
  }
#endif

  view_->LoadIcons(IDB_ITEMS, 16, scada::aui::Rgba{255, 0, 255});

  view_->SetSelectionChangeHandler([this] { OnSelectionChanged(); });

  view_->SetContextMenuHandler([this](const scada::aui::Point& point) {
    // Cross-platform AUI menu model (Windows, macOS, Wt) instead of the
    // Windows-only `IDR_TABLE_POPUP` resource menu.
    controller_delegate_.ShowPopupMenu(&table_menu_model_.model(),
                                       /*resource_id=*/0, point, true);
  });

  view_->SetDoubleClickHandler([this] { OnDoubleClick(); });

  view_->SetKeyPressHandler(
      [this](scada::aui::KeyCode key_code) { return OnKeyPressed(key_code); });

  selection_.multiple_handler = [this] { return GetMultipleSelection(); };

  delete_command_.execute_handler = [this] {
    view_->CloseEditor();
    DeleteSelection();
  };

  rename_command_.execute_handler = [this] {
    view_->OpenEditor(view_->GetCurrentRow());
  };

  move_up_command_.execute_handler = [this] { MoveRow(true); };
  move_down_command_.execute_handler = [this] { MoveRow(false); };

  sort_name_command_.execute_handler = [this] { model_->Sort(ID_SORT_NAME); };
  sort_channel_command_.execute_handler = [this] {
    model_->Sort(ID_SORT_CHANNEL);
  };

  // Enablement for the row commands, honoured by both the context menu and the
  // reshell toolbar. The grid's trailing "Enter expression" entry row (index
  // == row_count()) holds no data, so it never enables them.
  delete_command_.enabled_handler = [this] {
    for (int row : view_->GetSelectedRows()) {
      if (row >= 0 && row < model_->row_count())
        return true;
    }
    return false;
  };
  move_up_command_.enabled_handler = [this] {
    const int row = view_->GetCurrentRow();
    return row > 0 && row < model_->row_count();
  };
  move_down_command_.enabled_handler = [this] {
    const int row = view_->GetCurrentRow();
    return row >= 0 && row + 1 < model_->row_count();
  };
}

TableView::~TableView() {}

std::unique_ptr<UiView> TableView::Init(const WindowDefinition& definition) {
  for (auto& item : definition.items) {
    if (item.name_is("State")) {
      view_->RestoreState(item.attributes);

    } else if (item.name_is("Item")) {
      int ix = item.GetInt("ix", 0) - 1;
      if (ix == -1)
        ix = model_->row_count();
      auto path = item.GetString("path");
      model_->SetFormula(ix, std::string{path});
    }
  }

#if defined(UI_QT)
  // Opt-in reshell toolbar: the discoverable surfacing of the grid's row
  // commands (table-watch.html), complementing the right-click context menu.
  // MakeTableToolbar returns null under the legacy theme, keeping the bare
  // grid.
  toolbar_ = MakeTableToolbar(TableToolbarContext{
      .resolve_command = [this](unsigned command_id) -> CommandHandler* {
        // The view's own registry first (delete/move/sort), then the
        // shell's command surface (to-graph, CSV, print).
        if (CommandHandler* handler =
                command_registry_.GetCommandHandler(command_id)) {
          return handler;
        }
        return controller_delegate_.ResolveViewCommand(command_id);
      },
      .on_add_signal =
          [this] {
            // The grid's trailing "Enter expression" row (index row_count()).
            view_->OpenEditor(model_->row_count());
          }});
  if (toolbar_) {
    auto* container = new QWidget;
    auto* layout = new QVBoxLayout{container};
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(toolbar_);
    layout->addWidget(view_->CreateParentIfNecessary());
    return std::unique_ptr<UiView>{container};
  }
#endif

  return std::unique_ptr<UiView>{view_->CreateParentIfNecessary()};
}

void TableView::Save(WindowDefinition& definition) {
  definition.AddItem("State").attributes = view_->SaveState();

  for (int i = 0; i < model_->row_count(); i++) {
    TableRow* row = model_->GetRow(i);
    if (!row)
      continue;

    auto formula = row->GetFormula();
    if (formula.empty())
      continue;

    WindowItem& item = definition.AddItem("Item");
    item.SetInt("ix", i + 1);
    // WARNING: |SetString()| argument mustn't be an xvalue.
    item.SetString("path", formula);
  }
}

bool TableView::OnKeyPressed(scada::aui::KeyCode key_code) {
  switch (key_code) {
    case scada::aui::KeyCode::Enter:
      if (!view_->editing()) {
        OnDoubleClick();
        return true;
      }
      break;

    case scada::aui::KeyCode::Delete:
      if (!view_->editing()) {
        DeleteSelection();
        return true;
      }
      break;

    case scada::aui::KeyCode::Up:
    case scada::aui::KeyCode::Down:
#ifdef _WIN32
      if (GetAsyncKeyState(VK_CONTROL) < 0) {
        MoveRow(key_code == scada::aui::KeyCode::Up);
        return true;
      }
#endif
      break;
  }

  return false;
}

void TableView::OnDoubleClick() {
  int index = view_->GetCurrentRow();
  if (index < 0)
    return;

  auto* row = model_->GetRow(index);
  if (!row)
    return;

  if (row->is_blinking()) {
    row->timed_data().Acknowledge();

  } else {
    if (const auto& node = row->timed_data().node()) {
      controller_delegate_.ExecuteDefaultNodeCommand(node);
    }
  }
}

void TableView::AddContainedItem(const scada::NodeId& node_id, unsigned flags) {
  if (!(flags & APPEND))
    model_->Clear();

  auto node = node_service_.GetNode(node_id);
  if (IsInstanceOf(node, scada::data_items::id::DataGroupType)) {
    for (auto& child : node.targets(scada::id::HasComponent))
      AddContainedItem(child.node_id(), flags | APPEND);
    return;
  }

  if (node.node_class() != scada::NodeClass::Variable)
    return;

  int ix = model_->FindItem(node_id);
  if (ix != -1) {
    view_->SelectRow(ix, true);
    return;
  }

  ix = model_->row_count();
  model_->SetFormula(ix, MakeNodeIdFormula(node_id));

  view_->SelectRow(ix, true);

  controller_delegate_.SetModified(true);
}

void TableView::RemoveContainedItem(const scada::NodeId& node_id) {
  int ix;
  while ((ix = model_->FindItem(node_id)) >= 0)
    model_->DeleteRows(ix, 1);
}

void TableView::DeleteSelection() {
  for (int row : view_->GetSelectedRows())
    model_->DeleteRows(row, 1);
}

NodeIdSet TableView::GetMultipleSelection() {
  NodeIdSet node_ids;
  for (auto row_index : view_->GetSelectedRows()) {
    const auto* row = model_->GetRow(row_index);
    if (!row)
      continue;
    if (auto node_id = row->timed_data().node_id(); !node_id.is_null()) {
      node_ids.emplace(std::move(node_id));
    }
  }
  return node_ids;
}

NodeIdSet TableView::GetContainedItems() const {
  NodeIdSet items;
  for (int i = 0; i < model_->row_count(); i++) {
    const auto* row = model_->GetRow(i);
    if (!row)
      continue;
    if (auto node_id = row->timed_data().node_id(); !node_id.is_null()) {
      items.emplace(std::move(node_id));
    }
  }
  return items;
}

CommandHandler* TableView::GetCommandHandler(unsigned command_id) {
  return command_registry_.GetCommandHandler(command_id);
}

void TableView::MoveRow(bool up) {
  int row = view_->GetCurrentRow();
  if (row == -1)
    return;

  auto row2 = model_->MoveRow(row, up);
  if (row2 != -1)
    view_->SelectRow(row2, true);
}

void TableView::OnSelectionChanged() {
  auto rows = view_->GetSelectedRows();
  if (rows.empty()) {
    selection_.Clear();
  } else if (rows.size() == 1) {
    if (const auto* row = model_->GetRow(rows.front())) {
      selection_.SelectTimedData(row->timed_data());
    } else {
      selection_.Clear();
    }
  } else {
    selection_.SelectMultiple();
  }

#if defined(UI_QT)
  if (toolbar_)
    toolbar_->Refresh();
#endif
}

ExportModel::ExportData TableView::GetExportData() {
  return TableExportData{*model_, view_->columns(),
                         Range{0, model_->row_count()}};
}
