#include "configuration/tree/configuration_tree_node.h"

#include "configuration/tree/configuration_tree_model.h"
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
  model_.tree_node_map_.emplace(node_.node_id(), this);

  // Children load lazily — only via FetchMore (Qt calls it when the
  // user expands the row). Eagerly attaching already-loaded children
  // here used to be an optimization for re-opening the Objects panel,
  // but it walks the entire NodeService tree synchronously when the
  // address space starts pre-populated (e.g. screenshot generator on
  // top of AddressSpaceImpl3) — recursive ctor → AddChildren → ctor
  // overflows the stack. The lazy path covers the re-open case too:
  // Qt calls FetchMore again, and `node_.children_fetched()` short-
  // circuits the fetch when children are already in the address space.

  node_.Fetch(NodeFetchStatus::NodeOnly());
}

ConfigurationTreeNode::~ConfigurationTreeNode() {
  auto [first, last] = model_.tree_node_map_.equal_range(node_.node_id());
  auto i =
      std::find_if(first, last, [this](auto& p) { return p.second == this; });
  assert(i != last);
  model_.tree_node_map_.erase(i);
}

void ConfigurationTreeNode::AddChildren() {
  auto children = model_.node_service_tree_->GetChildren(node_);

  int n = 0;
  for (const auto& [reference_type_id, forward, child_node] : children) {
    auto new_child_tree_node =
        model_.CreateTreeNodeIfMatches(reference_type_id, forward, child_node);
    if (new_child_tree_node)
      Add(n++, std::move(new_child_tree_node));
  }
}

std::u16string ConfigurationTreeNode::GetText(int column_id) const {
  auto text = ToString16(node_.display_name());

  if (children_requested_ && !children_loaded_)
    text += u" [��������]";

  return text;
}

int ConfigurationTreeNode::GetIcon() const {
  return node_.node_class() == scada::NodeClass::Variable ? IMAGE_ITEM
                                                          : IMAGE_FOLDER;
}

void ConfigurationTreeNode::Changed() {
  model().TreeNodeChanged(this);
}

bool ConfigurationTreeNode::HasChildren() const {
  return model_.node_service_tree_->HasChildren(node_);
}

bool ConfigurationTreeNode::CanFetchMore() const {
  return !children_requested_;
}

void ConfigurationTreeNode::FetchMore() {
  assert(node_);
  assert(!children_requested_);

  LOG_INFO(model_.logger_) << "Load children"
                           << LOG_TAG("NodeId",
                                      NodeIdToScadaString(node_.node_id()));

  children_requested_ = true;

  if (node_.children_fetched()) {
    children_loaded_ = true;
    AddChildren();
    return;
  }

  node_.Fetch(NodeFetchStatus::NodeAndChildren(), [this](const NodeRef& node) {
    LOG_INFO(model_.logger_)
        << "Children"
        << LOG_TAG("NodeId", NodeIdToScadaString(node_.node_id()));

    children_loaded_ = true;
    Changed();
  });
}

// ConfigurationTreeRootNode

ConfigurationTreeRootNode::ConfigurationTreeRootNode(
    ConfigurationTreeModel& model,
    NodeRef tree)
    : ConfigurationTreeNode{model, {}, true, tree} {
  // One level of prefetch so the tree shows its first rows immediately
  // when the panel opens. Grandchildren load lazily through FetchMore —
  // stopping the recursion here is what prevents the ctor chain from
  // walking a pre-populated address space to a stack overflow.
  AddChildren();
}

std::u16string ConfigurationTreeRootNode::GetText(int column_id) const {
  return ToString16(node().display_name());
}

int ConfigurationTreeRootNode::GetIcon() const {
  return IMAGE_FOLDER;
}
