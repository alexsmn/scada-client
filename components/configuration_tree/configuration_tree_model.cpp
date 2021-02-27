#include "components/configuration_tree/configuration_tree_model.h"

#include "base/bind_util.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "core/event.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"

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

  root()->LoadChildren();
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
