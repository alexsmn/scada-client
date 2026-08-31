#include "modules/node_table/node_table_controller.h"

#include "aui/grid.h"
#include "aui/models/header_model.h"
#include "controller/controller_delegate.h"
#include "model/data_items_node_ids.h"
#include "model/node_id_util.h"
#include "modules/node_table/node_table_model.h"
#include "node_service/node_service.h"
#include "profile/profile.h"
#include "remote/session_proxy.h"
#include "resources/common_resources.h"
#include "services/task_manager.h"
#include "ui/common/client_utils.h"

#include <span>

#if defined(UI_QT)
#include "aui/grid.h"
#include "aui/qt/grid.h"
#include "aui/severity_colors.h"
#include "base/awaitable.h"
#include "model/security_node_ids.h"
#include "user_access/qt/users_grid_panel.h"
#include "user_access/users_grid.h"

#include <QPointer>

#include <utility>
#include <vector>
#endif

namespace {

std::span<
    const std::pair<unsigned /*command_id*/, scada::NodeId /*prop_decl_id*/>>
GetSortCommands() {
  static std::pair<unsigned, scada::NodeId> kSortCommands[] = {
      {ID_SORT_NONE, scada::NodeId{}},
      {ID_SORT_ALIAS, scada::data_items::id::DataItemType_Alias},
      {ID_SORT_CHANNEL, scada::data_items::id::DataItemType_Input1},
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
          property_service_,
          PropertyContext{context.executor_, context.node_service_,
                          context.task_manager_, context.dialog_service_})} {
  if (parent_node)
    model_->SetParentNode(parent_node);
}

NodeTableController::~NodeTableController() = default;

std::unique_ptr<UiView> NodeTableController::Init(
    const WindowDefinition& definition) {
  if (const WindowItem* item = definition.FindItem("Item")) {
    auto path = item->GetString("path");
    auto node_id = NodeIdFromScadaString(path);
    model_->SetParentNode(node_service_.GetNode(node_id));
  }

  model_->SetSorting(profile_.node_table.default_sort_property_id);

#if defined(UI_QT)
  // The Users administration table renders as the themed UsersGridPanel
  // (backlog 5.1) instead of the generic grid. Gated on the parent being the
  // Users folder so every other node table keeps the grid.
  if (model_->parent_node() &&
      model_->parent_node().node_id() == scada::security::id::Users) {
    {
      UsersGridPanel* panel = MakeUsersGridPanel();
      // Selecting a user row drives the shared SelectionModel, so the RBAC
      // inspector (UserAccessPanel) fills exactly as it does from the grid.
      QObject::connect(panel, &UsersGridPanel::UserActivated, panel,
                       [this](const scada::NodeId& user_id) {
                         selection_.SelectNode(node_service_.GetNode(user_id));
                       });
      // A row's actions (Set Password... / New / Delete) reuse the existing
      // selection commands: pop the standard context menu for the selected
      // user, exactly as the generic grid does.
      QObject::connect(panel, &UsersGridPanel::ActionsMenuRequested, panel,
                       [this](const QPoint& global_pos, bool right_click) {
                         // aui::Point is QPoint under UI_QT.
                         controller_delegate_.ShowPopupMenu(nullptr, global_pos,
                                                            right_click);
                       });
      // Populate the rows off the construction path; a QPointer guards a late
      // completion against a destroyed panel. The accounts come from the
      // standard UserManagement object rather than from this view's parent
      // folder — the folder is only what routed us here.
      CoSpawn(
          executor_,
          [executor = executor_, &node_service = node_service_,
           &attribute_service = attribute_service_,
           panel_ptr =
               QPointer<UsersGridPanel>{panel}]() mutable -> Awaitable<void> {
            auto rows = co_await BuildUsersGrid(executor, node_service,
                                                attribute_service);
            if (panel_ptr) {
              // nullopt is "the account list could not be read" — which a
              // non-administrator gets by design (Part 18 §5.2.1). Showing
              // an empty grid instead would say the server has no users.
              panel_ptr->ShowRows(rows ? *rows : std::vector<UserGridRow>{});
            }
            co_return;
          });
      return std::unique_ptr<UiView>{panel};
    }
  }
#endif

  grid_ = new scada::aui::Grid{
      model_,
      std::shared_ptr<scada::aui::HeaderModel>(model_, &model_->row_model()),
      std::shared_ptr<scada::aui::HeaderModel>(model_,
                                               &model_->column_model())};

  grid_->SetExpandAllowed(true);
  grid_->SetRowHeaderVisible(true);
  grid_->SetColumnHeaderHeight(19);
  grid_->SetRowHeaderWidth(70);

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

    // TODO: Investigate why `rows.front()` is not valid in some cases.
    NodeRef node = model_->node(rows.front());
    if (!node)
      return;

    selection_.SelectNode(node);
  });

  grid_->SetContextMenuHandler([this](const scada::aui::Point& point) {
    // Cross-platform AUI menu model (Windows, macOS) instead of the
    // Windows-only `IDR_GRID_POPUP` resource menu.
    controller_delegate_.ShowPopupMenu(&menu_model_.model(), point, true);
  });

  selection_.multiple_handler = [this] {
    NodeIdSet node_ids;
    for (auto row : grid_->GetSelectedRows())
      node_ids.emplace(model_->node(row).node_id());
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

  return std::unique_ptr<UiView>{grid_->CreateParentIfNecessary()};
}

void NodeTableController::Save(WindowDefinition& definition) {
  if (auto parent_node = model_->parent_node()) {
    auto path = NodeIdToScadaString(parent_node.node_id());
    definition.AddItem("Item").SetString("path", path);
  }

  // The reshell UsersGridPanel path leaves grid_ null; only the generic grid
  // persists column/sort state.
  if (grid_)
    definition.AddItem("State").attributes = grid_->SaveState();
}

CommandHandler* NodeTableController::GetCommandHandler(unsigned command_id) {
  return command_registry_.GetCommandHandler(command_id);
}

NodeRef NodeTableController::GetRootNode() const {
  return model_->parent_node();
}

void NodeTableController::SetSorting(const scada::NodeId& property_id) {
  profile_.node_table.default_sort_property_id = property_id;
  model_->SetSorting(property_id);
}

bool NodeTableController::IsWorking() const {
  return model_->loading();
}
