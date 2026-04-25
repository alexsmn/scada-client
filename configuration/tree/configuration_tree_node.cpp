#include "configuration/tree/configuration_tree_node.h"

#include "aui/translation.h"
#include "base/awaitable.h"
#include "base/callback_awaitable.h"
#include "configuration/tree/configuration_tree_model.h"
#include "model/node_id_util.h"

namespace {

Awaitable<NodeRef> FetchNodeAndChildrenAsync(std::shared_ptr<Executor> executor,
                                             NodeRef node) {
  auto [fetched_node] = co_await CallbackToAwaitable<NodeRef>(
      std::move(executor), [node = std::move(node)](auto callback) {
        node.Fetch(NodeFetchStatus::NodeAndChildren(), std::move(callback));
      });
  co_return std::move(fetched_node);
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
  model_.tree_node_map_.emplace(node_.node_id(), this);

  // Do not fetch here. Node fetch-status updates are reported back to
  // the tree as synthetic reference changes; creating a tree node and
  // immediately calling Fetch(NodeOnly) feeds the same update loop and
  // can recurse until the stack overflows during startup. Unfetched
  // nodes still render with a fallback display name until the regular
  // node-service pipeline fills them in.
}

ConfigurationTreeNode::~ConfigurationTreeNode() {
  auto [first, last] = model_.tree_node_map_.equal_range(node_.node_id());
  auto i =
      std::find_if(first, last, [this](auto& p) { return p.second == this; });
  assert(i != last);
  model_.tree_node_map_.erase(i);
}

int ConfigurationTreeNode::AddChildren() {
  auto children = model_.node_service_tree_->GetChildren(node_);

  int n = 0;
  for (const auto& [reference_type_id, forward, child_node] : children) {
    auto new_child_tree_node =
        model_.CreateTreeNodeIfMatches(reference_type_id, forward, child_node);
    if (new_child_tree_node)
      model_.Add(*this, n++, std::move(new_child_tree_node));
  }

  return n;
}

std::u16string ConfigurationTreeNode::GetText(int column_id) const {
  auto text = ToString16(node_.display_name());

  if (children_requested_ && !children_loaded_)
    text += u" [" + Translate("Loading") + u"]";

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
    const auto added_child_count = AddChildren();
    LOG_INFO(model_.logger_)
        << "Children materialized"
        << LOG_TAG("NodeId", NodeIdToScadaString(node_.node_id()))
        << LOG_TAG("AddedChildCount", added_child_count)
        << LOG_TAG("TotalChildCount", GetChildCount());
    return;
  }

  CoSpawn(model_.executor_,
          [executor = model_.executor_, lifetime_token = model_.GetLifetimeToken(),
           model = &model_, node = node_, node_id = node_.node_id(),
           reference_type_id = reference_type_id_,
           forward_reference = forward_reference_]() mutable
              -> Awaitable<void> {
            co_await ConfigurationTreeNode::CompleteFetchMoreAsync(
                std::move(executor), std::move(lifetime_token), *model,
                std::move(node), std::move(node_id),
                std::move(reference_type_id), forward_reference);
          });
}

Awaitable<void> ConfigurationTreeNode::CompleteFetchMoreAsync(
    std::shared_ptr<Executor> executor,
    std::weak_ptr<void> lifetime_token,
    ConfigurationTreeModel& model,
    NodeRef node,
    scada::NodeId node_id,
    scada::NodeId reference_type_id,
    bool forward_reference) {
  auto fetched_node =
      co_await FetchNodeAndChildrenAsync(std::move(executor), std::move(node));

  if (lifetime_token.expired()) {
    co_return;
  }

  auto* tree_node =
      model.FindTreeNode(node_id, reference_type_id, forward_reference);
  if (!tree_node) {
    LOG_INFO(model.logger_)
        << "Ignore stale children fetched callback"
        << LOG_TAG("NodeId", NodeIdToScadaString(fetched_node.node_id()));
    co_return;
  }

  LOG_INFO(model.logger_)
      << "Children fetched callback begin"
      << LOG_TAG("NodeId", NodeIdToScadaString(fetched_node.node_id()));

  tree_node->children_loaded_ = true;
  const auto added_child_count = tree_node->AddChildren();
  tree_node->Changed();

  LOG_INFO(model.logger_)
      << "Children fetched callback completed"
      << LOG_TAG("NodeId", NodeIdToScadaString(fetched_node.node_id()))
      << LOG_TAG("AddedChildCount", added_child_count)
      << LOG_TAG("TotalChildCount", tree_node->GetChildCount());
}

// ConfigurationTreeRootNode

ConfigurationTreeRootNode::ConfigurationTreeRootNode(
    ConfigurationTreeModel& model,
    NodeRef tree)
    : ConfigurationTreeNode{model, {}, true, tree} {
  // One level of prefetch so the tree shows its first rows immediately
  // when the panel opens. If the root's children are not in the local
  // address space yet, kick off the regular fetch path; grandchildren
  // still stay lazy through FetchMore, which is what prevents the ctor
  // chain from walking a pre-populated address space to a stack overflow.
  FetchMore();
}

std::u16string ConfigurationTreeRootNode::GetText(int column_id) const {
  return ToString16(node().display_name());
}

int ConfigurationTreeRootNode::GetIcon() const {
  return IMAGE_FOLDER;
}
