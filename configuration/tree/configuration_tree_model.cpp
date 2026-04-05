#include "configuration/tree/configuration_tree_model.h"

#include "model/node_id_util.h"
#include "node_service/node_service.h"

#include <span>

namespace {

inline bool DoesChildExist(std::span<const NodeServiceTree::ChildRef> children,
                           const scada::NodeId& reference_type_id,
                           bool forward,
                           const NodeRef& child_node) {
  return std::ranges::any_of(
      children, [&](const NodeServiceTree::ChildRef& child_ref) {
        return child_ref.reference_type_id == reference_type_id &&
               child_ref.forward == forward &&
               child_ref.child_node == child_node;
      });
}

}  // namespace

// ConfigurationTreeModel

ConfigurationTreeModel::ConfigurationTreeModel(
    ConfigurationTreeModelContext&& context)
    : ConfigurationTreeModelContext{std::move(context)} {}

void ConfigurationTreeModel::Init() {
  node_service_tree_->SetObserver(this);

  auto root_node = node_service_tree_->GetRoot();
  set_root(
      std::make_unique<ConfigurationTreeRootNode>(*this, std::move(root_node)));
}

ConfigurationTreeModel::~ConfigurationTreeModel() {
  set_root(nullptr);
}

void ConfigurationTreeModel::UpdateChildTreeNodes(
    const scada::NodeId& parent_id) {
  auto parent_tree_nodes = FindTreeNodes(parent_id);
  for (auto* parent_tree_node : parent_tree_nodes)
    UpdateChildTreeNodes(*parent_tree_node);
}

void ConfigurationTreeModel::UpdateChildTreeNodes(
    ConfigurationTreeNode& parent_tree_node) {
  LOG_INFO(logger_) << "Update child tree nodes"
                    << LOG_TAG("NodeId",
                               NodeIdToScadaString(
                                   parent_tree_node.node().node_id()));

  auto children = node_service_tree_->GetChildren(parent_tree_node.node());

  // Delete missing targets.
  for (int i = 0; i < parent_tree_node.GetChildCount();) {
    const auto& tree_node = parent_tree_node.GetChild(i);
    bool exists =
        DoesChildExist(children, tree_node.reference_type_id(),
                       tree_node.forward_reference(), tree_node.node());
    if (!exists)
      Remove(parent_tree_node, i);
    else
      ++i;
  }

  // Create missing targets.
  for (const auto& [reference_type_id, forward, node] : children) {
    auto tree_node = CreateTreeNodeIfMatches(reference_type_id, forward, node);
    if (tree_node) {
      Add(parent_tree_node, parent_tree_node.GetChildCount(),
          std::move(tree_node));
    }
  }
}

void ConfigurationTreeModel::DeleteTreeNodes(const scada::NodeId& node_id) {
  LOG_INFO(logger_) << "Delete tree nodes"
                    << LOG_TAG("NodeId", NodeIdToScadaString(node_id));

  // Remove tree nodes with missing references.
  for (const auto* tree_node : FindTreeNodes(node_id))
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

ConfigurationTreeNode* ConfigurationTreeModel::FindFirstTreeNode(
    const scada::NodeId& node_id) {
  auto i = tree_node_map_.lower_bound(node_id);
  return i != tree_node_map_.end() ? i->second : nullptr;
}

std::vector<ConfigurationTreeNode*> ConfigurationTreeModel::FindTreeNodes(
    const scada::NodeId& node_id) {
  auto [first, last] = tree_node_map_.equal_range(node_id);
  std::vector<ConfigurationTreeNode*> tree_nodes;
  for (auto i = first; i != last; ++i)
    tree_nodes.emplace_back(i->second);
  return tree_nodes;
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

  return CreateTreeNode(reference_type_id, forward_reference, node);
}

void ConfigurationTreeModel::OnNodeDeleted(const scada::NodeId& node_id) {
  DeleteTreeNodes(node_id);
}

void ConfigurationTreeModel::OnNodeChildrenChanged(
    const scada::NodeId& node_id) {
  UpdateChildTreeNodes(node_id);
}

void ConfigurationTreeModel::OnNodeModelChanged(const scada::NodeId& node_id) {
  auto [first, last] = tree_node_map_.equal_range(node_id);
  for (auto i = first; i != last; ++i)
    i->second->OnModelChanged();
}

void ConfigurationTreeModel::OnNodeSemanticsChanged(
    const scada::NodeId& node_id) {
  auto [first, last] = tree_node_map_.equal_range(node_id);
  for (auto i = first; i != last; ++i)
    TreeNodeChanged(i->second);
}
