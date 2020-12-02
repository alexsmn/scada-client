#include "components/node_table/node_table_controller.h"

#include "client_utils.h"
#include "common_resources.h"
#include "components/node_table/node_table_model.h"
#include "controller_delegate.h"
#include "controls/grid.h"
#include "model/data_items_node_ids.h"
#include "model/node_id_util.h"
#include "node_service/node_service.h"
#include "remote/session_proxy.h"
#include "services/profile.h"
#include "services/property_defs.h"
#include "services/task_manager.h"

#if defined(UI_QT)
#include "controls/qt/grid.h"
#elif defined(UI_VIEWS)
#include "ui/views/controls/textfield/combo_textfield.h"
#endif

namespace {

scada::NodeId GetSortCommandPropertyId(unsigned command_id) {
  switch (command_id) {
    case ID_SORT_NONE:
      return {};
    case ID_SORT_ALIAS:
      return data_items::id::DataItemType_Alias;
    case ID_SORT_CHANNEL:
      return data_items::id::DataItemType_Input1;
    default:
      assert(false);
      return {};
  }
}

}  // namespace

// NodeTableController

NodeTableController::NodeTableController(const ControllerContext& context,
                                         const NodeRef& parent_node)
    : ControllerContext{context},
      model_{std::make_unique<NodeTableModel>(
          PropertyContext{context.node_service_, context.task_manager_,
                          context.dialog_service_})} {
  if (parent_node)
    model_->SetParentNode(parent_node);
}

NodeTableController::~NodeTableController() {}

UiView* NodeTableController::Init(const WindowDefinition& definition) {
  if (const WindowItem* item = definition.FindItem("Item")) {
    auto path = item->GetString("path");
    auto node_id = NodeIdFromScadaString(path);
    model_->SetParentNode(node_service_.GetNode(node_id));
  }

  model_->SetSorting(profile_.node_table.default_sort_property_id);

  grid_.reset(new Grid(*model_, model_->row_model(), model_->column_model()));

  grid_->SetExpandAllowed(true);
  grid_->SetRowHeaderVisible(true);
  grid_->SetColumnHeaderHeight(19);
  grid_->SetRowHeaderWidth(70);

#if defined(UI_VIEWS)
  // grid_->set_controller(this);
  grid_->set_allow_column_select(true);
  grid_->SelectCell(0, 0, true);
#endif

  grid_->SetSelectionChangeHandler([this] {
    auto rows = grid_->GetSelectedRows();
    if (rows.empty()) {
      selection_.Clear();
      return;
    }

    if (rows.size() >= 2) {
      selection_.SelectMultiple();
      return;
    }

    auto node = model_->nodes()[rows.front()];
    assert(node);

    selection_.SelectNode(node);
  });

  grid_->SetContextMenuHandler([this](const UiPoint& point) {
    controller_delegate_.ShowPopupMenu(0, point, true);
  });

  selection_.multiple_handler = [this] {
    NodeIdSet node_ids;
    for (auto row : grid_->GetSelectedRows())
      node_ids.emplace(model_->nodes()[row].node_id());
    return node_ids;
  };

  if (auto* state = definition.FindItem("State"))
    grid_->RestoreState(state->attributes);

  return grid_->CreateParentIfNecessary();
}

void NodeTableController::Save(WindowDefinition& definition) {
  if (auto parent_node = model_->parent_node()) {
    auto path = NodeIdToScadaString(parent_node.node_id());
    definition.AddItem("Item").SetString("path", path);
  }

  definition.AddItem("State").attributes = grid_->SaveState();
}

CommandHandler* NodeTableController::GetCommandHandler(unsigned command_id) {
  switch (command_id) {
    case ID_RENAME:
    case ID_SORT_NONE:
    case ID_SORT_ALIAS:
    case ID_SORT_CHANNEL:
      return this;
  }

  return Controller::GetCommandHandler(command_id);
}

bool NodeTableController::IsCommandEnabled(unsigned command_id) const {
  return __super::IsCommandEnabled(command_id);
}

bool NodeTableController::IsCommandChecked(unsigned command_id) const {
  switch (command_id) {
    case ID_SORT_NONE:
    case ID_SORT_ALIAS:
    case ID_SORT_CHANNEL:
      return model_->sort_property_id() == GetSortCommandPropertyId(command_id);
    default:
      return false;
  }
}

void NodeTableController::ExecuteCommand(unsigned command) {
  switch (command) {
    case ID_RENAME:
      if (auto index = grid_->GetCurrentIndex(); index.is_valid())
        grid_->OpenEditor(index);
      break;
    case ID_SORT_NONE:
    case ID_SORT_ALIAS:
    case ID_SORT_CHANNEL:
      SetSorting(GetSortCommandPropertyId(command));
      break;
    default:
      __super::ExecuteCommand(command);
      break;
  }
}

NodeRef NodeTableController::GetRootNode() const {
  return model_->parent_node();
}

#if defined(UI_VIEWS)
bool NodeTableController::OnKeyPressed(views::GridView& sender,
                                       ui::KeyboardCode key_code) {
  // Clear selection when Delete is pressed.
  if (key_code == ui::VKEY_DELETE && !grid_->editing()) {
    ui::GridRange range = grid_->GetSelectionRange();
    if (!range.empty()) {
      for (int row = range.row(); row <= range.last_row(); row++)
        for (int col = range.column(); col <= range.last_column(); col++)
          model_->SetCellText(row, col, std::wstring());
      return true;
    }
  }

  return false;
}

void NodeTableController::ShowHeaderContextMenu(gfx::Point point) {
  __super::ShowHeaderContextMenu(point);

  // TODO: Doesn't work.
}

void NodeTableController::OnGridColumnClicked(views::GridView& sender,
                                              int index) {
  __super::OnGridColumnClicked(sender, index);

  // TODO: Doesn't work.
}
#endif

void NodeTableController::SetSorting(const scada::NodeId& property_id) {
  profile_.node_table.default_sort_property_id = property_id;
  model_->SetSorting(property_id);
}
