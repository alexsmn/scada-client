#include "components/node_table/node_table_controller.h"

#include "client_utils.h"
#include "common_resources.h"
#include "components/node_table/node_table_model.h"
#include "controller_delegate.h"
#include "controls/grid.h"
#include "controls/models/header_model.h"
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

base::span<
    const std::pair<unsigned /*command_id*/, scada::NodeId /*prop_decl_id*/>>
GetSortCommands() {
  static std::pair<unsigned, scada::NodeId> kSortCommands[] = {
      {ID_SORT_NONE, scada::NodeId{}},
      {ID_SORT_ALIAS, data_items::id::DataItemType_Alias},
      {ID_SORT_CHANNEL, data_items::id::DataItemType_Input1},
  };
  return kSortCommands;
}

}  // namespace

// NodeTableController

NodeTableController::NodeTableController(const ControllerContext& context,
                                         const NodeRef& parent_node)
    : ControllerContext{context},
      model_{std::make_unique<NodeTableModel>(
          executor_,
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

  grid_ = new aui::Grid{
      model_, std::shared_ptr<aui::HeaderModel>(model_, &model_->row_model()),
      std::shared_ptr<aui::HeaderModel>(model_, &model_->column_model())};

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

  grid_->SetContextMenuHandler([this](const aui::Point& point) {
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

  command_registry_.AddCommand(Command{ID_RENAME}.set_execute_handler([this] {
    if (auto index = grid_->GetCurrentIndex(); index.is_valid())
      grid_->OpenEditor(index);
  }));

  for (const auto& [command_id, prop_decl_id] : GetSortCommands()) {
    command_registry_.AddCommand(
        Command{command_id}
            .set_execute_handler([this, prop_decl_id = prop_decl_id] {
              SetSorting(prop_decl_id);
            })
            .set_checked_handler([this, prop_decl_id = prop_decl_id] {
              return model_->sort_property_id() == prop_decl_id;
            }));
  }

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
  return command_registry_.GetCommandHandler(command_id);
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
          model_->SetCellText(row, col, std::u16string());
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

bool NodeTableController::IsWorking() const {
  return model_->loading();
}
