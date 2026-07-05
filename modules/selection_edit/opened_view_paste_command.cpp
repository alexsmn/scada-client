#include "selection_edit/opened_view_paste_command.h"

#include "base/check.h"
#include "clipboard/clipboard_util.h"
#include "controller/controller.h"
#include "controller/selection_model.h"
#include "net/net_executor_adapter.h"
#include "node_service/node_util.h"
#include "resources/common_resources.h"
#include "services/create_tree.h"
#include "services/task_manager.h"

#include <stdexcept>

OpenedViewPasteCommand::OpenedViewPasteCommand(
    OpenedViewPasteCommandContext&& context)
    : OpenedViewPasteCommandContext{std::move(context)} {}

OpenedViewPasteCommand::~OpenedViewPasteCommand() = default;

CommandHandler* OpenedViewPasteCommand::GetCommandHandler(unsigned command_id) {
  return command_id == ID_PASTE &&
                 session_service_.HasPrivilege(scada::Privilege::Configure)
             ? this
             : nullptr;
}

bool OpenedViewPasteCommand::IsCommandEnabled(unsigned command_id) const {
  base::Check(command_id == ID_PASTE);
  auto* selection_model = controller_.GetSelectionModel();
  return selection_model &&
         session_service_.HasPrivilege(scada::Privilege::Configure) &&
         GetPasteParentNode(node_service_, create_tree_,
                            selection_model->node(), controller_.GetRootNode());
}

void OpenedViewPasteCommand::ExecuteCommand(unsigned command_id) {
  base::Check(command_id == ID_PASTE);
  CoSpawn(executor_, cancelation_, [this]() mutable -> Awaitable<void> {
    co_await PasteFromClipboardAsync();
    co_return;
  });
}

Awaitable<void> OpenedViewPasteCommand::PasteFromClipboardAsync() {
  if (!session_service_.HasPrivilege(scada::Privilege::Configure)) {
    throw std::runtime_error{"Configure privilege is required to paste"};
  }

  const auto* selection_model = controller_.GetSelectionModel();
  if (!selection_model) {
    throw std::runtime_error{"Selection model is required to paste"};
  }

  const auto& parent_node =
      GetPasteParentNode(node_service_, create_tree_, selection_model->node(),
                         controller_.GetRootNode());
  if (!parent_node) {
    throw std::runtime_error{"No valid paste parent is available"};
  }

  co_await PasteNodesFromClipboard(task_manager_, parent_node.node_id());
}
