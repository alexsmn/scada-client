#include "components/table/table_view.h"

#include "aui/table.h"
#include "client_utils.h"
#include "common/formula_util.h"
#include "common_resources.h"
#include "components/table/table_model.h"
#include "components/table/table_row.h"
#include "controller/contents_observer.h"
#include "controller/controller_delegate.h"
#include "model/data_items_node_ids.h"
#include "model/scada_node_ids.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"
#include "controller/selection_model.h"
#include "services/dialog_service.h"
#include "profile/profile.h"
#include "string_const.h"

// TableView

TableView::TableView(const ControllerContext& context)
    : ControllerContext{context},
      model_{std::make_shared<TableModel>(
          TableModelContext{timed_data_service_, node_event_provider_, profile_,
                            dialog_service_, blinker_manager_})} {
  model_->item_changed_ = [this](const scada::NodeId& item_id, bool added) {
    NotifyContainedItemChanged(item_id, added);
  };

  std::vector<aui::TableColumn> columns = {
      {TableModel::COLUMN_TITLE, kDisplayNameAttributeString, 150,
       aui::TableColumn::LEFT},
      {TableModel::COLUMN_VALUE, kValueTitle, 100, aui::TableColumn::RIGHT},
      {TableModel::COLUMN_SOURCE_TIMESTAMP, kSourceTimestampTitle, 170,
       aui::TableColumn::LEFT, aui::TableColumn::DataType::DateTime},
      {TableModel::COLUMN_SERVER_TIMESTAMP, kServerTimestampTitle, 170,
       aui::TableColumn::LEFT, aui::TableColumn::DataType::DateTime},
      {TableModel::COLUMN_CHANGE_TIME, u"Время изменения", 170,
       aui::TableColumn::LEFT, aui::TableColumn::DataType::DateTime},
      {TableModel::COLUMN_EVENT, u"Событие", 200, aui::TableColumn::LEFT},
  };

  view_ = new aui::Table{model_, std::move(columns)};

  view_->LoadIcons(IDB_ITEMS, 16, aui::Rgba{255, 0, 255});

  view_->SetSelectionChangeHandler([this] { OnSelectionChanged(); });

  view_->SetContextMenuHandler([this](const aui::Point& point) {
    controller_delegate_.ShowPopupMenu(IDR_TABLE_POPUP, point, true);
  });

  view_->SetDoubleClickHandler([this] { OnDoubleClick(); });

  view_->SetKeyPressHandler(
      [this](aui::KeyCode key_code) { return OnKeyPressed(key_code); });

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
}

TableView::~TableView() {}

UiView* TableView::Init(const WindowDefinition& definition) {
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

  return view_->CreateParentIfNecessary();
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

bool TableView::OnKeyPressed(aui::KeyCode key_code) {
  switch (key_code) {
    case aui::KeyCode::Enter:
      if (!view_->editing()) {
        OnDoubleClick();
        return true;
      }
      break;

    case aui::KeyCode::Delete:
      if (!view_->editing()) {
        DeleteSelection();
        return true;
      }
      break;

    case aui::KeyCode::Up:
    case aui::KeyCode::Down:
      if (GetAsyncKeyState(VK_CONTROL) < 0) {
        MoveRow(key_code == aui::KeyCode::Up);
        return true;
      }
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
  if (IsInstanceOf(node, data_items::id::DataGroupType)) {
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
}

ExportModel::ExportData TableView::GetExportData() {
  return TableExportData{*model_, view_->columns(),
                         Range{0, model_->row_count()}};
}
