#include "configuration/tree/configuration_tree_drop_handler.h"

#include "common/formula_util.h"
#include "configuration/tree/configuration_tree_node.h"
#include "item_drag_data.h"
#include "model/data_items_node_ids.h"
#include "model/devices_node_ids.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"
#include "services/create_tree.h"
#include "services/task_manager.h"

namespace {

DropAction MakeMoveDropAction(TaskManager& task_manager,
                              const scada::NodeId& node_id,
                              const scada::NodeId& old_parent_id,
                              const scada::NodeId& new_parent_id) {
  return [=, &task_manager] {
    if (!old_parent_id.is_null()) {
      task_manager.PostDeleteReference(scada::id::Organizes, old_parent_id,
                                       node_id);
    }

    if (!new_parent_id.is_null()) {
      task_manager.PostAddReference(scada::id::Organizes, new_parent_id,
                                    node_id);
    }

    return aui::DragDropTypes::DRAG_MOVE;
  };
}

DropAction MakeCreateDataItemAction(TaskManager& task_manager,
                                    const scada::NodeId& parent_id,
                                    scada::NodeAttributes attributes,
                                    std::string formula,
                                    bool control_item) {
  return [=, &task_manager] {
    auto type_definition_id =
        (control_item || attributes.data_type == scada::id::Boolean)
            ? data_items::id::DiscreteItemType
            : data_items::id::AnalogItemType;

    auto channel_prop_id = control_item ? data_items::id::DataItemType_Output
                                        : data_items::id::DataItemType_Input1;

    task_manager.PostInsertTask(
        {.type_definition_id = std::move(type_definition_id),
         .parent_id = parent_id,
         .attributes = std::move(attributes),
         .properties = {{std::move(channel_prop_id), std::move(formula)}}});

    return aui::DragDropTypes::DRAG_COPY;
  };
}

DropAction MakeAssignChannelAction(TaskManager& task_manager,
                                   const scada::NodeId& node_id,
                                   std::string formula,
                                   bool control_item) {
  return [=, &task_manager]() mutable {
    auto channel_prop_id = control_item ? data_items::id::DataItemType_Output
                                        : data_items::id::DataItemType_Input1;

    task_manager.PostUpdateTask(
        node_id, /*attributes=*/{},
        /*properties=*/{{std::move(channel_prop_id), std::move(formula)}});

    return aui::DragDropTypes::DRAG_LINK;
  };
}

}  // namespace

ConfigurationTreeDropHandler::ConfigurationTreeDropHandler(
    ConfigurationTreeDropHandlerContext&& context)
    : ConfigurationTreeDropHandlerContext{std::move(context)} {}

int ConfigurationTreeDropHandler::GetDropAction(
    const scada::NodeId& dragging_id,
    const ConfigurationTreeNode* target_node,
    DropAction& action) {
  if (!target_node)
    return aui::DragDropTypes::DRAG_NONE;

  auto dragging_node = node_service_.GetNode(dragging_id);
  if (!dragging_node)
    return aui::DragDropTypes::DRAG_NONE;

  // Dropping of IEC-61850 channel into id::DataGroupType causes
  // creation of new id::DataItemType.
  bool is_iec61850_channel =
      IsInstanceOf(dragging_node, devices::id::Iec61850DataVariableType) ||
      IsInstanceOf(dragging_node, devices::id::Iec61850ControlObjectType);

  if (is_iec61850_channel && IsSubtypeOf(target_node->node().type_definition(),
                                         data_items::id::DataGroupType)) {
    action = MakeCreateDataItemAction(
        task_manager_, /*parent_id=*/target_node->node().node_id(),
        /*attributes=*/
        {.browse_name = dragging_node.browse_name(),
         .display_name = dragging_node.display_name(),
         .data_type = dragging_node.data_type().node_id()},
        /*formula=*/MakeNodeIdFormula(dragging_node.node_id()),
        /*control_item=*/
        IsInstanceOf(dragging_node, devices::id::Iec61850ControlObjectType));

    return aui::DragDropTypes::DRAG_COPY;
  }

  // Dropping of IEC-61850 channel on id::DataItem assigns its' channel.
  if (is_iec61850_channel &&
      IsInstanceOf(target_node->node(), data_items::id::DataItemType)) {
    auto formula = MakeNodeIdFormula(dragging_node.node_id());

    action = MakeAssignChannelAction(
        task_manager_, target_node->node().node_id(), std::move(formula),
        IsInstanceOf(dragging_node, devices::id::Iec61850ControlObjectType));
    return aui::DragDropTypes::DRAG_LINK;
  }

  // Dropping a node to a node that can contain the node type causes move.
  {
    auto type_definition = target_node->node().type_definition();
    if (!type_definition ||
        !create_tree_.CanCreate(dragging_node, type_definition)) {
      return aui::DragDropTypes::DRAG_NONE;
    }

    if (target_node->node() != dragging_node &&
        target_node->node() != dragging_node.parent()) {
      auto old_parent_id = dragging_node.parent().node_id();
      auto new_parent_id = target_node->node().node_id();

      action = MakeMoveDropAction(task_manager_, dragging_id, old_parent_id,
                                  new_parent_id);

      return aui::DragDropTypes::DRAG_MOVE;
    }
  }

  return aui::DragDropTypes::DRAG_NONE;
}

int ConfigurationTreeDropHandler::GetDropAction(
    const DragData& drag_data,
    const ConfigurationTreeNode* target_node,
    DropAction& action) {
  ItemDragData item_drag_data;
  if (!item_drag_data.Load(drag_data)) {
    return aui::DragDropTypes::DRAG_NONE;
  }

  return GetDropAction(item_drag_data.item_id(), target_node, action);
}
