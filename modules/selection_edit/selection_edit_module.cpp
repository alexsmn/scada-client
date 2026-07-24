#include "selection_edit/selection_edit_module.h"

#include "aui/dialog_service.h"
#include "aui/key_codes.h"
#include "aui/translation.h"
#include "base/awaitable.h"
#include "base/u16format.h"
#include "clipboard/clipboard_util.h"
#include "controller/action.h"
#include "controller/command_registry.h"
#include "controller/command_ui_registry.h"
#include "controller/selection_model.h"
#include "core/selection_command_context.h"
#include "main_window/opened_view/opened_view_command_registry.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"
#include "resources/common_resources.h"
#include "selection_edit/opened_view_paste_command.h"
#include "services/task_manager.h"
#include "ui/common/client_utils.h"

#include <algorithm>
#include <ranges>
#include <utility>

namespace {

void CopyToClipboard(const SelectionCommandContext& context,
                     NodeService& node_service) {
  std::vector<NodeRef> nodes;

  if (context.selection.multiple()) {
    for (const auto& node_id : context.selection.GetMultipleNodeIds()) {
      const auto& node = node_service.GetNode(node_id);
      nodes.emplace_back(node);
      GetNodesRecursive(node, nodes);
    }

  } else if (const auto& node = context.selection.node()) {
    nodes.emplace_back(node);
  }

  if (!nodes.empty()) {
    CopyNodesToClipboard(nodes);
  }
}

void DeleteSelection(AnyExecutor executor,
                     const SelectionCommandContext& context,
                     NodeService& node_service,
                     TaskManager& task_manager,
                     scada::SessionService& session_service) {
  if (!session_service.HasPermission(scada::Permission::kDeleteNode)) {
    return;
  }

  std::vector<NodeRef> nodes;

  if (context.selection.multiple()) {
    auto node_ids = context.selection.GetMultipleNodeIds();
    nodes.reserve(node_ids.size());
    std::ranges::transform(node_ids, std::back_inserter(nodes),
                           [&node_service](const scada::NodeId& node_id) {
                             return node_service.GetNode(node_id);
                           });

  } else if (auto node = context.selection.node()) {
    nodes.emplace_back(std::move(node));
  }

  if (nodes.empty()) {
    return;
  }

  auto message = nodes.size() == 1
                     ? u16format(L"Are you sure you want to delete {}?",
                                 nodes.front().display_name().text)
                     : u16format(L"Are you sure you want to delete {} items?",
                                 nodes.size());

  CoSpawn(executor,
          [&task_manager, &dialog_service = context.dialog_service,
           message = std::move(message),
           nodes = std::move(nodes)]() mutable -> Awaitable<void> {
            auto result = co_await dialog_service.RunMessageBox(
                message, Translate("Delete"), MessageBoxMode::QuestionYesNo);
            if (result != MessageBoxResult::Yes) {
              co_return;
            }
            for (const NodeRef& node : nodes) {
              DeleteTreeRecordsRecursive(task_manager, node);
            }
            co_return;
          });
}

}  // namespace

SelectionEditModule::SelectionEditModule(SelectionEditModuleContext&& context)
    : SelectionEditModuleContext{std::move(context)} {
  ui_command_registry_.AddAction(Action{
      .command_id_ = ID_COPY,
      .category_ = CATEGORY_EDIT,
      .title_ = Translate("Copy"),
      .image_id_ = IDB_COPY,
      .shortcut_ =
          Shortcut{scada::aui::ControlModifier, scada::aui::KeyCode::C}});
  ui_command_registry_.AddAction(Action{
      .command_id_ = ID_PASTE,
      .category_ = CATEGORY_EDIT,
      .title_ = Translate("Paste"),
      .image_id_ = IDB_PASTE,
      .shortcut_ =
          Shortcut{scada::aui::ControlModifier, scada::aui::KeyCode::V}});
  ui_command_registry_.AddAction(
      Action{.command_id_ = ID_DELETE,
             .category_ = CATEGORY_EDIT,
             .title_ = Translate("Delete"),
             .image_id_ = IDB_DELETE,
             .shortcut_ = Shortcut{scada::aui::KeyCode::Delete}});

  opened_view_commands_.AddFactory(
      [](const OpenedViewCommandFactoryContext& context) {
        return std::make_unique<OpenedViewPasteCommand>(
            OpenedViewPasteCommandContext{
                .executor_ = context.executor_,
                .session_service_ = context.session_service_,
                .node_service_ = context.node_service_,
                .task_manager_ = context.task_manager_,
                .create_tree_ = context.create_tree_,
                .controller_ = context.controller_});
      });

  selection_commands_.AddCommand(BasicCommand<SelectionCommandContext>{
      .command_id = ID_COPY,
      .execute_handler =
          [&node_service =
               node_service_](const SelectionCommandContext& context) {
            CopyToClipboard(context, node_service);
          },
      .enabled_handler =
          [](const SelectionCommandContext& context) {
            return !context.selection.empty();
          },
      .available_handler =
          [&session_service =
               session_service_](const SelectionCommandContext& context) {
            return session_service.HasAccessRight(
                       scada::AccessRight::kConfigure) &&
                   !context.selection.empty();
          }});
  selection_commands_.AddCommand(BasicCommand<SelectionCommandContext>{
      .command_id = ID_DELETE,
      .execute_handler =
          [executor = executor_, &node_service = node_service_,
           &task_manager = task_manager_, &session_service = session_service_](
              const SelectionCommandContext& context) {
            DeleteSelection(executor, context, node_service, task_manager,
                            session_service);
          },
      .enabled_handler =
          [](const SelectionCommandContext& context) {
            return !context.selection.empty();
          },
      .available_handler =
          [&session_service =
               session_service_](const SelectionCommandContext& context) {
            return session_service.HasPermission(
                       scada::Permission::kDeleteNode) &&
                   !context.selection.empty();
          }});
}
