#include "components/configuration_tree/configuration_tree_model.h"

#include "base/strings/sys_string_conversions.h"
#include "common/formula_util.h"
#include "common/node_service.h"
#include "common/node_util.h"
#include "common/scada_node_ids.h"
#include "core/node_management_service.h"
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

    return ui::DragDropTypes::DRAG_MOVE;
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
            ? id::DiscreteItemType
            : id::AnalogItemType;
    auto channel_prop_id =
        control_item ? id::DataItemType_Output : id::DataItemType_Input1;
    task_manager.PostInsertTask(
        {}, parent_id, std::move(type_definition_id), std::move(attributes),
        {{std::move(channel_prop_id), std::move(formula)}});

    return ui::DragDropTypes::DRAG_COPY;
  };
}

DropAction MakeAssignChannelAction(TaskManager& task_manager,
                                   const scada::NodeId& node_id,
                                   std::string formula,
                                   bool control_item) {
  return [=, &task_manager] {
    auto channel_prop_id =
        control_item ? id::DataItemType_Output : id::DataItemType_Input1;
    task_manager.PostUpdateTask(
        node_id, {}, {{std::move(channel_prop_id), std::move(formula)}});

    return ui::DragDropTypes::DRAG_LINK;
  };
}

}  // namespace

// ConfigurationTreeNode

ConfigurationTreeNode::ConfigurationTreeNode(ConfigurationTreeModel& model,
                                             const NodeRef& data_node)
    : model_{model}, data_node_{data_node} {
  assert(&data_node_);
  model_.node_map_[data_node_.id()] = this;
}

ConfigurationTreeNode::~ConfigurationTreeNode() {
  model_.node_map_.erase(data_node_.id());
}

int ConfigurationTreeNode::GetChildCount() const {
  const_cast<ConfigurationTreeNode*>(this)->Load();
  return ui::TreeNode<ConfigurationTreeNode>::GetChildCount();
}

void ConfigurationTreeNode::Load() {
  assert(data_node_);

  if (loaded_)
    return;

  loaded_ = true;

  int n = 0;
  for (auto& reference_type_id : model_.reference_type_ids_) {
    const auto& children = data_node_.targets(reference_type_id, true);
    for (auto data_child : children) {
      if (auto child = model_.CreateNodeIfMatches(data_child)) {
        auto* child_ptr = child.get();
        Add(n, std::move(child));
      }
    }
  }
}

base::string16 ConfigurationTreeNode::GetText(int column_id) const {
  auto text = ToString16(data_node_.display_name());

  auto fetch_status = data_node_.fetch_status();
  bool fetched = fetch_status.node_fetched && fetch_status.children_fetched;
  if (!fetched)
    text += L" [Загрузка]";

  return text;
}

int ConfigurationTreeNode::GetIcon() const {
  return data_node_.node_class() == scada::NodeClass::Variable ? IMAGE_ITEM
                                                               : IMAGE_FOLDER;
}

void ConfigurationTreeNode::Changed() {
  model().TreeNodeChanged(this);
}

// ConfigurationTreeRootNode

ConfigurationTreeRootNode::ConfigurationTreeRootNode(
    ConfigurationTreeModel& model,
    NodeRef tree)
    : ConfigurationTreeNode(model, tree) {}

base::string16 ConfigurationTreeRootNode::GetText(int column_id) const {
  return data_node().display_name();
}

int ConfigurationTreeRootNode::GetIcon() const {
  return IMAGE_FOLDER;
}

// ConfigurationTreeModel

ConfigurationTreeModel::ConfigurationTreeModel(
    NodeService& node_service,
    TaskManager& task_manager,
    NodeRef root_node,
    std::vector<scada::NodeId> reference_type_ids,
    std::vector<scada::NodeId> type_definition_ids)
    : node_service_{node_service},
      task_manager_{task_manager},
      reference_type_ids_{std::move(reference_type_ids)},
      type_definition_ids_{std::move(type_definition_ids)} {
  set_root(std::make_unique<ConfigurationTreeRootNode>(*this, std::move(root_node)));
}

ConfigurationTreeModel::~ConfigurationTreeModel() {
  node_service_.Unsubscribe(*this);

  set_root(NULL);
}

void ConfigurationTreeModel::Init() {
  node_service_.Subscribe(*this);

  root()->Load();
}

void ConfigurationTreeModel::UpdateNode(const scada::ModelChangeEvent& event) {
  auto node = node_service_.GetNode(event.node_id);

  NodeRef::Reference parent_ref =
      node.reference(scada::id::HierarchicalReferences, false);
  if (!parent_ref.target)
    return;

  bool matches = false;
  for (auto& reference_type_id : reference_type_ids_) {
    if (IsSubtypeOf(parent_ref.reference_type, reference_type_id)) {
      matches = true;
      break;
    }
  }
  if (!matches)
    return;

  ConfigurationTreeNode* old_tree_node = FindNode(event.node_id);
  if (old_tree_node && old_tree_node == root()) {
    old_tree_node->OnModelChanged(event);
    TreeNodeChanged(old_tree_node);
    return;
  }

  ConfigurationTreeNode* new_parent_node = FindNode(parent_ref.target.id());
  if (!new_parent_node || !new_parent_node->loaded())
    return;

  if (old_tree_node)
    old_tree_node->OnModelChanged(event);

  if (old_tree_node && old_tree_node->parent() == new_parent_node) {
    TreeNodeChanged(old_tree_node);
    return;
  }

  std::unique_ptr<ConfigurationTreeNode> tree_node;
  // Remove node from the old parent.
  if (old_tree_node)
    tree_node = Remove(*old_tree_node->parent(),
                       old_tree_node->parent()->IndexOfChild(*old_tree_node));

  if (!tree_node)
    tree_node = CreateNodeIfMatches(node);
  if (tree_node) {
    Add(*new_parent_node, new_parent_node->GetChildCount(),
        std::move(tree_node));
  }
}

void ConfigurationTreeModel::OnModelChanged(
    const scada::ModelChangeEvent& event) {
  if (event.verb & scada::ModelChangeEvent::NodeDeleted) {
    if (ConfigurationTreeNode* tree_node = FindNode(event.node_id)) {
      tree_node->OnModelChanged(event);
      Remove(*tree_node->parent(),
             tree_node->parent()->IndexOfChild(*tree_node));
    }

  } else {
    UpdateNode(event);
  }
}

void ConfigurationTreeModel::OnNodeSemanticChanged(
    const scada::NodeId& node_id) {
  if (ConfigurationTreeNode* tree_node = FindNode(node_id))
    TreeNodeChanged(tree_node);
}

std::unique_ptr<ConfigurationTreeNode> ConfigurationTreeModel::CreateNode(
    const NodeRef& data_node) {
  assert(data_node);
  return std::make_unique<ConfigurationTreeNode>(*this, data_node);
}

std::unique_ptr<ConfigurationTreeNode>
ConfigurationTreeModel::CreateNodeIfMatches(const NodeRef& data_node) {
  assert(data_node);
  assert(!FindNode(data_node.id()));

  if (!type_definition_ids_.empty()) {
    bool matches = false;
    const auto type_definition = data_node.type_definition();
    for (auto& filter_type_definition_id : type_definition_ids_) {
      if (IsSubtypeOf(type_definition, filter_type_definition_id)) {
        matches = true;
        break;
      }
    }
    if (!matches)
      return nullptr;
  }

  return CreateNode(data_node);
}

int ConfigurationTreeModel::GetDropAction(const scada::NodeId& dragging_id,
                                          ConfigurationTreeNode*& target_node,
                                          DropAction& action) {
  if (!target_node)
    return ui::DragDropTypes::DRAG_NONE;

  auto dragging_node = node_service_.GetNode(dragging_id);
  if (!dragging_node)
    return ui::DragDropTypes::DRAG_NONE;

  // Dropping of IEC-61850 channel into id::DataGroupType causes
  // creation of new id::DataItemType.
  bool is_iec61850_channel =
      IsInstanceOf(dragging_node, id::Iec61850DataVariableType) ||
      IsInstanceOf(dragging_node, id::Iec61850ControlObjectType);
  if (is_iec61850_channel &&
      IsSubtypeOf(target_node->data_node().type_definition(),
                  id::DataGroupType)) {
    scada::NodeAttributes attributes;
    attributes.browse_name = dragging_node.browse_name();
    attributes.display_name = dragging_node.display_name();
    attributes.data_type = dragging_node.data_type().id();
    auto formula = MakeNodeIdFormula(dragging_node.id());
    action = MakeCreateDataItemAction(
        task_manager_, target_node->data_node().id(), std::move(attributes),
        std::move(formula),
        IsInstanceOf(dragging_node, id::Iec61850ControlObjectType));
    return ui::DragDropTypes::DRAG_COPY;
  }

  // Dropping of IEC-61850 channel on id::DataItem assigns its' channel.
  if (is_iec61850_channel &&
      IsInstanceOf(target_node->data_node(), id::DataItemType)) {
    auto formula = MakeNodeIdFormula(dragging_node.id());
    action = MakeAssignChannelAction(
        task_manager_, target_node->data_node().id(), std::move(formula),
        IsInstanceOf(dragging_node, id::Iec61850ControlObjectType));
    return ui::DragDropTypes::DRAG_LINK;
  }

  // Dropping a node to a node that can contain the node type causes move.
  {
    for (; target_node; target_node = target_node->parent()) {
      auto type_definition = target_node->data_node().type_definition();
      if (type_definition && CanCreate(dragging_node, type_definition))
        break;
    }

    if (target_node && target_node->data_node() != dragging_node &&
        target_node->data_node() != dragging_node.parent()) {
      auto old_parent_id = dragging_node.parent().id();
      auto new_parent_id = target_node->data_node().id();
      action = MakeMoveDropAction(task_manager_, dragging_id, old_parent_id,
                                  new_parent_id);
      return ui::DragDropTypes::DRAG_MOVE;
    }
  }

  return ui::DragDropTypes::DRAG_NONE;
}

ConfigurationTreeNode* ConfigurationTreeModel::FindNode(
    const scada::NodeId& node_id) {
  if (node_id.is_null())
    return nullptr;
  auto i = node_map_.find(node_id);
  return i != node_map_.end() ? i->second : nullptr;
}
