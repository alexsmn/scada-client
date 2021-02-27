#include "components/configuration_tree/configuration_tree_node.h"

#include "components/configuration_tree/configuration_tree_model.h"

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
  const_cast<ConfigurationTreeNode*>(this)->LoadChildren();
  return ui::TreeNode<ConfigurationTreeNode>::GetChildCount();
}

void ConfigurationTreeNode::LoadChildren() {
  assert(node_);

  if (children_loaded_)
    return;

  children_loaded_ = true;

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
