#include "components/configuration_tree/configuration_tree_node.h"

#include "components/configuration_tree/configuration_tree_model.h"
#include "model/node_id_util.h"

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

  node_.Fetch(NodeFetchStatus::NodeOnly(), nullptr);
}

ConfigurationTreeNode::~ConfigurationTreeNode() {
  auto [first, last] = model_.tree_node_map_.equal_range(node_.node_id());
  auto i =
      std::find_if(first, last, [this](auto& p) { return p.second == this; });
  assert(i != last);
  model_.tree_node_map_.erase(i);
}

void ConfigurationTreeNode::LoadChildren() {
  assert(node_);

  if (children_loaded_)
    return;

  LOG_INFO(model_.logger_) << "Load children"
                           << LOG_TAG("NodeId",
                                      NodeIdToScadaString(node_.node_id()));

  children_loaded_ = true;

  auto children = model_.node_service_tree_->GetChildren(node_);

  int n = 0;
  for (const auto& [reference_type_id, forward, child_node] : children) {
    auto new_child_tree_node =
        model_.CreateTreeNodeIfMatches(reference_type_id, forward, child_node);
    if (new_child_tree_node)
      Add(n++, std::move(new_child_tree_node));
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

bool ConfigurationTreeNode::CanFetchMore() const {
  return !children_loaded_;
}

void ConfigurationTreeNode::FetchMore() {
  LoadChildren();
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
