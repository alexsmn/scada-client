#include "components/configuration_tree/configuration_tree_model.h"

#include "base/bind_util.h"
#include "base/strings/sys_string_conversions.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "common/formula_util.h"
#include "core/event.h"
#include "core/node_management_service.h"
#include "model/data_items_node_ids.h"
#include "model/devices_node_ids.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"
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
            ? data_items::id::DiscreteItemType
            : data_items::id::AnalogItemType;
    auto channel_prop_id = control_item ? data_items::id::DataItemType_Output
                                        : data_items::id::DataItemType_Input1;
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
    auto channel_prop_id = control_item ? data_items::id::DataItemType_Output
                                        : data_items::id::DataItemType_Input1;
    task_manager.PostUpdateTask(
        node_id, {}, {{std::move(channel_prop_id), std::move(formula)}});

    return ui::DragDropTypes::DRAG_LINK;
  };
}

}  // namespace

// ConfigurationTreeNode

ConfigurationTreeNode::ConfigurationTreeNode(ConfigurationTreeModel& model,
                                             scada::NodeId reference_type_id,
                                             bool forward_reference,
                                             NodeRef node)
    : model_{model},
      reference_type_id_{std::move(reference_type_id)},
      forward_reference_{forward_reference},
      node_{std::move(node)} {
  assert(&node_);
  model_.tree_node_map_.emplace(node_.node_id(), this);
}

ConfigurationTreeNode::~ConfigurationTreeNode() {
  auto [first, last] = model_.tree_node_map_.equal_range(node_.node_id());
  auto i =
      std::find_if(first, last, [this](auto& p) { return p.second == this; });
  assert(i != last);
  model_.tree_node_map_.erase(i);
}

int ConfigurationTreeNode::GetChildCount() const {
  const_cast<ConfigurationTreeNode*>(this)->Load();
  return ui::TreeNode<ConfigurationTreeNode>::GetChildCount();
}

void ConfigurationTreeNode::Load() {
  assert(node_);

  if (loaded_)
    return;

  loaded_ = true;

  int n = 0;
  for (const auto& [reference_type_id, forward] : model_.reference_filter_) {
    const auto& targets = forward ? node_.targets(reference_type_id)
                                  : node_.inverse_targets(reference_type_id);
    for (const auto& node : targets) {
      auto tree_node =
          model_.CreateTreeNodeIfMatches(reference_type_id, forward, node);
      if (tree_node)
        Add(n++, std::move(tree_node));
    }
  }

  node_.Fetch(NodeFetchStatus::NodeAndChildren(), nullptr);
}

std::wstring ConfigurationTreeNode::GetText(int column_id) const {
  auto text = ToString16(node_.display_name());

  bool fetched = node_.fetched() && node_.children_fetched();
  if (!fetched)
    text += L" [Загрузка]";

  return text;
}

int ConfigurationTreeNode::GetIcon() const {
  return node_.node_class() == scada::NodeClass::Variable ? IMAGE_ITEM
                                                          : IMAGE_FOLDER;
}

void ConfigurationTreeNode::Changed() {
  model().TreeNodeChanged(this);
}

// ConfigurationTreeRootNode

ConfigurationTreeRootNode::ConfigurationTreeRootNode(
    ConfigurationTreeModel& model,
    NodeRef tree)
    : ConfigurationTreeNode{model, {}, true, tree} {}

std::wstring ConfigurationTreeRootNode::GetText(int column_id) const {
  return node().display_name();
}

int ConfigurationTreeRootNode::GetIcon() const {
  return IMAGE_FOLDER;
}

// ConfigurationTreeModel

ConfigurationTreeModel::ConfigurationTreeModel(
    ConfigurationTreeModelContext&& context)
    : ConfigurationTreeModelContext{std::move(context)} {
  set_root(std::make_unique<ConfigurationTreeRootNode>(*this, root_node_));
}

ConfigurationTreeModel::~ConfigurationTreeModel() {
  node_service_.Unsubscribe(*this);

  set_root(NULL);
}

void ConfigurationTreeModel::Init() {
  node_service_.Subscribe(*this);

  root()->Load();
}

void ConfigurationTreeModel::DeleteMissingTreeNodes(
    const scada::NodeId& node_id) {
  // Remove tree nodes with missing references.
  auto [first, last] = tree_node_map_.equal_range(node_id);
  for (auto i = first; i != last;) {
    auto& tree_node = *i->second;
    ++i;

    if (auto* parent_tree_node = tree_node.parent()) {
      bool exists = parent_tree_node->node().has_target(
          tree_node.reference_type_id(), tree_node.forward_reference(),
          node_id);
      if (!exists)
        Remove(*parent_tree_node, parent_tree_node->IndexOfChild(tree_node));
    }
  }
}

void ConfigurationTreeModel::UpdateChildTreeNodes(
    const scada::NodeId& parent_id) {
  for (auto* parent_tree_node : FindTreeNodes(parent_id)) {
    // Delete missing targets.
    for (int i = 0; i < parent_tree_node->GetChildCount();) {
      auto& tree_node = parent_tree_node->GetChild(i);
      bool exists = parent_tree_node->node().has_target(
          tree_node.reference_type_id(), tree_node.forward_reference(),
          tree_node.node().node_id());
      if (!exists)
        Remove(*parent_tree_node, i);
      else
        ++i;
    }

    // Create missing targets.
    for (const auto& [reference_type_id, forward] : reference_filter_) {
      const auto& targets =
          forward ? parent_tree_node->node().targets(reference_type_id)
                  : parent_tree_node->node().inverse_targets(reference_type_id);
      for (const auto& node : targets) {
        auto tree_node =
            CreateTreeNodeIfMatches(reference_type_id, forward, node);
        if (tree_node) {
          Add(*parent_tree_node, parent_tree_node->GetChildCount(),
              std::move(tree_node));
        }
      }
    }
  }
}

void ConfigurationTreeModel::DeleteTreeNodes(const scada::NodeId& node_id) {
  // Remove tree nodes with missing references.
  for (auto* tree_node : FindTreeNodes(node_id))
    Remove(*tree_node->parent(), tree_node->parent()->IndexOfChild(*tree_node));
}

ConfigurationTreeNode* ConfigurationTreeModel::FindTreeNode(
    const scada::NodeId& node_id,
    const scada::NodeId& reference_type_id,
    bool forward_reference) {
  auto [first, last] = tree_node_map_.equal_range(node_id);
  for (auto i = first; i != last; ++i) {
    auto& node = *i->second;
    if (node.reference_type_id() == reference_type_id &&
        node.forward_reference() == forward_reference)
      return &node;
  }
  return nullptr;
}

std::vector<ConfigurationTreeNode*> ConfigurationTreeModel::FindTreeNodes(
    const scada::NodeId& node_id) {
  auto [first, last] = tree_node_map_.equal_range(node_id);
  std::vector<ConfigurationTreeNode*> tree_nodes;
  for (auto i = first; i != last; ++i)
    tree_nodes.emplace_back(i->second);
  return tree_nodes;
}

void ConfigurationTreeModel::OnModelChanged(
    const scada::ModelChangeEvent& event) {
  if (event.verb & scada::ModelChangeEvent::NodeDeleted) {
    DeleteTreeNodes(event.node_id);

  } else {
    if (event.verb & (scada::ModelChangeEvent::ReferenceAdded |
                      scada::ModelChangeEvent::ReferenceDeleted))
      UpdateChildTreeNodes(event.node_id);
  }

  auto [first, last] = tree_node_map_.equal_range(event.node_id);
  for (auto i = first; i != last; ++i)
    i->second->OnModelChanged(event);
}

void ConfigurationTreeModel::OnNodeSemanticChanged(
    const scada::NodeId& node_id) {
  auto task_runner = base::SequencedTaskRunnerHandle::Get();
  task_runner->PostTask(FROM_HERE, BindLambda([this, node_id] {
                          auto [first, last] =
                              tree_node_map_.equal_range(node_id);
                          for (auto i = first; i != last; ++i)
                            TreeNodeChanged(i->second);
                        }));
}

std::unique_ptr<ConfigurationTreeNode> ConfigurationTreeModel::CreateTreeNode(
    const scada::NodeId& reference_type_id,
    bool forward_reference,
    const NodeRef& node) {
  assert(node);
  return std::make_unique<ConfigurationTreeNode>(*this, reference_type_id,
                                                 forward_reference, node);
}

std::unique_ptr<ConfigurationTreeNode>
ConfigurationTreeModel::CreateTreeNodeIfMatches(
    const scada::NodeId& reference_type_id,
    bool forward_reference,
    const NodeRef& node) {
  assert(node);

  if (FindTreeNode(node.node_id(), reference_type_id, forward_reference))
    return nullptr;

  if (!type_definition_ids_.empty()) {
    bool matches = false;
    const auto& type_definition = node.type_definition();
    for (auto& filter_type_definition_id : type_definition_ids_) {
      if (IsSubtypeOf(type_definition, filter_type_definition_id)) {
        matches = true;
        break;
      }
    }
    if (!matches)
      return nullptr;
  }

  return CreateTreeNode(reference_type_id, forward_reference, node);
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
      IsInstanceOf(dragging_node, devices::id::Iec61850DataVariableType) ||
      IsInstanceOf(dragging_node, devices::id::Iec61850ControlObjectType);
  if (is_iec61850_channel && IsSubtypeOf(target_node->node().type_definition(),
                                         data_items::id::DataGroupType)) {
    scada::NodeAttributes attributes;
    attributes.browse_name = dragging_node.browse_name();
    attributes.display_name = dragging_node.display_name();
    attributes.data_type = dragging_node.data_type().node_id();
    auto formula = MakeNodeIdFormula(dragging_node.node_id());
    action = MakeCreateDataItemAction(
        task_manager_, target_node->node().node_id(), std::move(attributes),
        std::move(formula),
        IsInstanceOf(dragging_node, devices::id::Iec61850ControlObjectType));
    return ui::DragDropTypes::DRAG_COPY;
  }

  // Dropping of IEC-61850 channel on id::DataItem assigns its' channel.
  if (is_iec61850_channel &&
      IsInstanceOf(target_node->node(), data_items::id::DataItemType)) {
    auto formula = MakeNodeIdFormula(dragging_node.node_id());
    action = MakeAssignChannelAction(
        task_manager_, target_node->node().node_id(), std::move(formula),
        IsInstanceOf(dragging_node, devices::id::Iec61850ControlObjectType));
    return ui::DragDropTypes::DRAG_LINK;
  }

  // Dropping a node to a node that can contain the node type causes move.
  {
    for (; target_node; target_node = target_node->parent()) {
      auto type_definition = target_node->node().type_definition();
      if (type_definition && CanCreate(dragging_node, type_definition))
        break;
    }

    if (target_node && target_node->node() != dragging_node &&
        target_node->node() != dragging_node.parent()) {
      auto old_parent_id = dragging_node.parent().node_id();
      auto new_parent_id = target_node->node().node_id();
      action = MakeMoveDropAction(task_manager_, dragging_id, old_parent_id,
                                  new_parent_id);
      return ui::DragDropTypes::DRAG_MOVE;
    }
  }

  return ui::DragDropTypes::DRAG_NONE;
}
