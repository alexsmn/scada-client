#include "components/configuration_tree/views/configuration_tree_drag_drop_controller_views.h"

#include "components/configuration_tree/configuration_tree_drop_handler.h"
#include "components/configuration_tree/configuration_tree_node.h"
#include "core/privileges.h"
#include "core/session_service.h"
#include "ui/views/widget/widget.h"
#include "views/controls/tree/tree_view.h"
#include "views/item_drag_data.h"

ConfigurationTreeDragDropControllerViews::
    ConfigurationTreeDragDropControllerViews(
        views::TreeView& tree_view,
        ConfigurationTreeDropHandler& drop_handler,
        scada::SessionService& session_service)
    : tree_view_{tree_view},
      drop_handler_{drop_handler},
      session_service_{session_service} {}

void ConfigurationTreeDragDropControllerViews::StartDrag(void* node) {
  ConfigurationTreeNode* cfg_node = static_cast<ConfigurationTreeNode*>(node);
  if (!cfg_node->node())
    return;

  scada::NodeId item_id = cfg_node->node().node_id();
  if (item_id.is_null())
    return;

  ui::OSExchangeData data;
  ItemDragData item_data(item_id);
  item_data.Save(data);

  if (auto* widget = tree_view_.GetWidget()) {
    widget->RunShellDrag(data,
                         DROPEFFECT_MOVE | DROPEFFECT_COPY | DROPEFFECT_LINK);
  }
}

bool ConfigurationTreeDragDropControllerViews::CanDrop(
    const ui::OSExchangeData& data) {
  return session_service_.HasPrivilege(scada::Privilege::Configure) &&
         data.HasCustomFormat(ItemDragData::GetCustomFormat());
}

void ConfigurationTreeDragDropControllerViews::OnDragEntered(
    const ui::DropTargetEvent& event) {
  assert(dragging_item_id_.is_null());

  ItemDragData item_data;
  if (item_data.Load(event.data()))
    dragging_item_id_ = item_data.item_id();
}

int ConfigurationTreeDragDropControllerViews::OnDragUpdated(
    const ui::DropTargetEvent& event) {
  ConfigurationTreeNode* target_node = reinterpret_cast<ConfigurationTreeNode*>(
      tree_view_.GetNodeAt(event.location()));
  int result =
      drop_handler_.GetDropAction(dragging_item_id_, target_node, drop_action_);
  if (result == ui::DragDropTypes::DRAG_NONE) {
    target_node = nullptr;
    drop_action_ = nullptr;
  }
  tree_view_.SetDropTargetNode(target_node);
  return result;
}

void ConfigurationTreeDragDropControllerViews::OnDragDone() {
  dragging_item_id_ = scada::NodeId();
  drop_action_ = nullptr;
  tree_view_.SetDropTargetNode(NULL);
}

int ConfigurationTreeDragDropControllerViews::OnPerformDrop(
    const ui::DropTargetEvent& event) {
  assert(!dragging_item_id_.is_null());

  auto drop_action = std::move(drop_action_);

  drop_action_ = nullptr;
  dragging_item_id_ = scada::NodeId();
  tree_view_.SetDropTargetNode(NULL);

  return drop_action();
}
