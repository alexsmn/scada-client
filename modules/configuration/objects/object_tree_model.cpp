#include "configuration/objects/object_tree_model.h"

#include "aui/translation.h"
#include "base/check.h"
#include "configuration/tree/node_service_tree_impl.h"
#include "model/data_items_node_ids.h"
#include "node_service/node_util.h"
#include "scada/standard_node_ids.h"

namespace {

Awaitable<NodeRef> FetchNodeOnlyAsync(AnyExecutor executor, NodeRef node) {
  co_return co_await node.Fetch(NodeFetchStatus::NodeOnly);
}

// A cyclic or pathological type graph must not spin the walk below forever.
// Real OPC UA type chains are a handful of hops deep.
constexpr int kMaxTypeChainHops = 32;

// Fetching the instance is not enough to classify it. IsInstanceOf walks the
// type definition's supertype chain, and every hop reads a *type* node that may
// be unfetched or not even resident; an unfetched type reports no supertype, so
// e.g. an AnalogItemType instance fails the DataItemType test. That test is
// one-shot — nothing re-runs it — so the row keeps its placeholder, never
// subscribes, and its Value column stays empty for the rest of the session.
// Pull the chain in first.
Awaitable<void> FetchTypeChainAsync(NodeRef node) {
  NodeRef type = node.type_definition();
  for (int hop = 0; type && hop < kMaxTypeChainHops; ++hop) {
    if (!type.fetched())
      type = co_await type.Fetch(NodeFetchStatus::NodeOnly);
    if (!type)
      break;
    type = type.supertype();
  }
}

}  // namespace

class ObjectTreeModel::ObjectTreeNode : public ConfigurationTreeNode {
 public:
  ObjectTreeNode(ObjectTreeModel& model,
                 scada::NodeId reference_type_id,
                 bool forward_reference,
                 const NodeRef& node)
      : ConfigurationTreeNode{model, std::move(reference_type_id),
                              forward_reference, node} {}

  virtual void OnModelChanged() override { Changed(); }

  // No icon. The Objects explorer is the operator's tree, and there every row
  // is either a container or a data item — a distinction the twisty and the
  // indentation already make. A per-row glyph would only repeat it, while the
  // one thing that varies, live quality, rides the status dot
  // (VisibleNodeModel::GetStatusColor) as docs/client/ux/principles.md §5 requires:
  // state is a dot, never artwork. This is what
  // docs/product/ui-mockups/screens/operator-shell.html shows — its rows carry no icon
  // element at all, unlike the engineering tree in config-workbench.html,
  // which mixes links, devices and signals at one level and still needs a kind
  // glyph.
  //
  // Overridden rather than left to the base, which classifies by node class
  // and always yields a glyph. HardwareTreeModel keeps that behaviour.
  virtual int GetIcon() const override { return scada::aui::kNoIcon; }
};

ObjectTreeModel::ObjectTreeModel(ObjectTreeModelContext&& context)
    : ObjectTreeModelContext{std::move(context)},
      ConfigurationTreeModel{::ConfigurationTreeModelContext{
          .executor_ = ObjectTreeModelContext::executor_,
          .node_service_tree_ =
              node_service_tree_factory_(NodeServiceTreeImplContext{
                  .executor_ = ObjectTreeModelContext::executor_,
                  .node_service_ = ObjectTreeModelContext::node_service_,
                  .root_node_ = ObjectTreeModelContext::root_,
                  .reference_filter_ = {{scada::id::Organizes, true}},
                  .leaf_type_definition_ids_ =
                      {scada::data_items::id::DataItemType},
              })}},
      visible_node_model_{
          timed_data_service_,
          profile_,
          [this](void* tree_node) { TreeNodeChanged(tree_node); },
      } {}

int ObjectTreeModel::GetColumnCount() const {
  return 2;
}

std::u16string ObjectTreeModel::GetColumnText(int column_id) const {
  return column_id == 0 ? Translate("Name") : Translate("Value");
}

int ObjectTreeModel::GetColumnPreferredSize(int column_id) const {
  return column_id == 0 ? 200 : 0;
}

std::unique_ptr<ConfigurationTreeNode> ObjectTreeModel::CreateTreeNode(
    const scada::NodeId& reference_type_id,
    bool forward_reference,
    const NodeRef& node) {
  return std::make_unique<ObjectTreeNode>(*this, reference_type_id,
                                          forward_reference, node);
}

std::u16string ObjectTreeModel::GetText(void* tree_node, int column_id) {
  if (column_id == 1)
    return visible_node_model_.GetText(tree_node);
  else
    return ConfigurationTreeModel::GetText(tree_node, column_id);
}

scada::aui::Color ObjectTreeModel::GetTextColor(void* tree_node,
                                                int column_id) {
  if (column_id == 1)
    return visible_node_model_.GetTextColor(tree_node);
  else
    return ConfigurationTreeModel::GetTextColor(tree_node, column_id);
}

scada::aui::Color ObjectTreeModel::GetBackgroundColor(void* tree_node,
                                                      int column_id) {
  if (column_id == 1)
    return visible_node_model_.GetBackgroundColor(tree_node);
  else
    return ConfigurationTreeModel::GetBackgroundColor(tree_node, column_id);
}

std::optional<scada::aui::Color> ObjectTreeModel::GetStatusColor(
    void* tree_node) {
  // Quality dot on the name column, from the node's live value.
  return visible_node_model_.GetStatusColor(tree_node);
}

bool ObjectTreeModel::IsMonospaceColumn(int column_id) const {
  // The live Value column renders in the monospace value font under the token
  // themes so updating digits stay tabular (design-language.md §3).
  return column_id == 1;
}

void ObjectTreeModel::SetNodeVisible(void* tree_node, bool visible) {
  auto visible_node = visible ? CreateVisibleNode(tree_node) : nullptr;
  visible_node_model_.SetNode(tree_node, std::move(visible_node));
}

std::shared_ptr<VisibleNode> ObjectTreeModel::CreateVisibleNode(
    void* tree_node) {
  auto& configuration_tree_node =
      *static_cast<ConfigurationTreeNode*>(tree_node);
  auto node = configuration_tree_node.node();

  if (node.fetched())
    return CreateFetchedVisibleNode(node);

  auto proxy_visible_node = std::make_shared<ProxyVisibleNode>();
  CoSpawn(ObjectTreeModelContext::executor_,
          [executor = ObjectTreeModelContext::executor_,
           lifetime_token = GetLifetimeToken(), model = this,
           proxy_visible_node, tree_node, node, node_id = node.node_id(),
           reference_type_id = configuration_tree_node.reference_type_id(),
           forward_reference =
               configuration_tree_node.forward_reference()]() mutable
              -> Awaitable<void> {
            co_await ObjectTreeModel::CompleteVisibleNodeFetchAsync(
                std::move(executor), std::move(lifetime_token), *model,
                std::move(proxy_visible_node), tree_node, std::move(node),
                std::move(node_id), std::move(reference_type_id),
                forward_reference);
          });

  return proxy_visible_node;
}

Awaitable<void> ObjectTreeModel::CompleteVisibleNodeFetchAsync(
    AnyExecutor executor,
    std::weak_ptr<void> lifetime_token,
    ObjectTreeModel& model,
    std::shared_ptr<ProxyVisibleNode> proxy_visible_node,
    void* tree_node,
    NodeRef node,
    scada::NodeId node_id,
    scada::NodeId reference_type_id,
    bool forward_reference) {
  auto fetched_node =
      co_await FetchNodeOnlyAsync(std::move(executor), std::move(node));

  if (lifetime_token.expired()) {
    co_return;
  }

  // The classification below reads the type chain; make sure it is there.
  co_await FetchTypeChainAsync(fetched_node);

  if (lifetime_token.expired()) {
    co_return;
  }

  if (!model.FindTreeNode(node_id, reference_type_id, forward_reference)) {
    co_return;
  }

  if (!model.visible_node_model_.HasNode(tree_node, proxy_visible_node)) {
    co_return;
  }

  auto visible_node = model.CreateFetchedVisibleNode(fetched_node);
  proxy_visible_node->SetUnderlyingNode(std::move(visible_node));
  co_return;
}

std::shared_ptr<VisibleNode> ObjectTreeModel::CreateFetchedVisibleNode(
    const NodeRef& node) {
  scada::base::Check(node.fetched());

  if (IsInstanceOf(node, scada::data_items::id::DataItemType)) {
    return std::make_shared<DataItemVisibleNode>(timed_data_service_,
                                                 blinker_manager_, node);

  } else if (IsInstanceOf(node, scada::data_items::id::DataGroupType)) {
    return std::make_shared<DataGroupVisibleNode>(timed_data_service_, node);

  } else {
    return nullptr;
  }
}
