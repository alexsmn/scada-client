#include "configuration/tree/node_service_tree_impl.h"

#include "base/any_executor_dispatch.h"
#include "base/check.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"
#include "scada/event.h"
#include "scada/node_class.h"
#include "scada/standard_node_ids.h"
#include <algorithm>

namespace {

// True when the tree follows nothing but *forward* `Organizes`.
//
// OPC UA Part 3 §7.11 Organizes,
// https://reference.opcfoundation.org/Core/Part3/v105/docs/7.11 (verified
// 2026-08-26): "The SourceNode of References of this type shall be an Object,
// ObjectType or a View." A Variable can therefore never be the source of an
// Organizes Reference, so under this filter every Variable row is a leaf by
// specification — no type system consulted and no round trip taken.
//
// Inverse entries disqualify the filter, because the same paragraph says "The
// TargetNode of this ReferenceType can be of any NodeClass": a Variable
// followed *inversely* does have a target (whatever organizes it), so it is not
// a leaf. An empty filter disqualifies it too — it configures no traversal at
// all, and answering "leaf" for it would be a statement about nothing.
bool FollowsOnlyForwardOrganizes(
    const NodeServiceTreeImplContext::ReferenceFilter& reference_filter) {
  return !reference_filter.empty() &&
         std::ranges::all_of(reference_filter, [](const auto& entry) {
           const auto& [reference_type_id, forward] = entry;
           return forward && reference_type_id == scada::id::Organizes;
         });
}

}  // namespace

NodeServiceTreeImpl::NodeServiceTreeImpl(NodeServiceTreeImplContext&& context)
    : NodeServiceTreeImplContext{std::move(context)},
      follows_only_forward_organizes_{
          FollowsOnlyForwardOrganizes(reference_filter_)},
      model_changed_connection_{node_service_.SubscribeModelChanged(
          [this](const scada::ModelChangeEvent& event) {
            OnModelChanged(event);
          })},
      node_semantic_changed_connection_{
          node_service_.SubscribeNodeSemanticChanged(
              [this](const scada::NodeId& node_id) {
                OnNodeSemanticChanged(node_id);
              })} {}

NodeServiceTreeImpl ::~NodeServiceTreeImpl() = default;

NodeRef NodeServiceTreeImpl::GetRoot() const {
  return root_node_;
}

bool NodeServiceTreeImpl::HasChildren(const NodeRef& node) const {
  // The NodeClass rule goes first, because it is the only one that can answer
  // when the row is *created* — which is what decides whether the operator ever
  // sees an expander on a leaf.
  //
  // The type-system test below cannot. It walks the type definition's supertype
  // chain, and a chain hop reads a *type* node: an unfetched type reports no
  // supertype, so `IsSubtypeOf(AnalogItemType, DataItemType)` is false until
  // AnalogItemType itself has been fetched. Nothing fetches it at row creation
  // — the fetch that publishes a parent's children pulls each *child* NodeOnly
  // (common/node_service/v3/node_model_impl.cpp) and never its type — so the
  // expander appeared, then vanished a round trip later when the visible-node
  // path happened to pull the chain in
  // (ObjectTreeModel::CompleteVisibleNodeFetchAsync). That same child fetch
  // does leave the NodeClass resident, so this rule holds on the first paint.
  //
  // See FollowsOnlyForwardOrganizes for why the filter has to be checked: the
  // rule is a fact about Organizes, not about Variables.
  if (follows_only_forward_organizes_ &&
      node.node_class() == scada::NodeClass::Variable) {
    return false;
  }

  // `node` is an instance, so the leaf test has to go through its type
  // definition. `IsSubtypeOf` walks HasSubtype, which only type nodes carry —
  // handing it an instance made every comparison false, so the whole
  // `leaf_type_definition_ids_` mechanism was dead and every TS/TIT row (and
  // every file row) offered an expander onto nothing. `IsMatchingNode` below
  // has always resolved the type definition first; this now matches it.
  return std::ranges::none_of(
      leaf_type_definition_ids_,
      [&node](const scada::NodeId& leaf_type_definition_id) {
        return IsInstanceOf(node, leaf_type_definition_id);
      });
}

std::vector<NodeServiceTreeImpl::ChildRef> NodeServiceTreeImpl::GetChildren(
    const NodeRef& node) const {
  if (!HasChildren(node)) {
    return {};
  }

  std::vector<ChildRef> children;

  for (const auto& [reference_type_id, forward] : reference_filter_) {
    const auto& targets = forward ? node.targets(reference_type_id)
                                  : node.inverse_targets(reference_type_id);
    for (const auto& child_node : targets) {
      if (IsMatchingNode(child_node)) {
        children.emplace_back(reference_type_id, forward, child_node);
      }
    }
  }

  return children;
}

bool NodeServiceTreeImpl::IsMatchingNode(const NodeRef& node) const {
  scada::base::Check(node);

  if (!type_definition_ids_.empty()) {
    bool matches = std::ranges::any_of(
        type_definition_ids_,
        [type_definition = node.type_definition()](
            const scada::NodeId& filter_type_definition_id) {
          return IsSubtypeOf(type_definition, filter_type_definition_id);
        });
    if (!matches)
      return false;
  }

  return true;
}

void NodeServiceTreeImpl::SetObserver(Observer* observer) {
  observer_ = observer;
}

void NodeServiceTreeImpl::OnModelChanged(const scada::ModelChangeEvent& event) {
  // TODO: weak_ptr
  Dispatch(executor_, [this, event] {
    if (!observer_)
      return;

    if (event.verb & scada::ModelChangeEvent::NodeDeleted) {
      observer_->OnNodeDeleted(event.node_id);

    } else {
      if (event.verb & (scada::ModelChangeEvent::ReferenceAdded |
                        scada::ModelChangeEvent::ReferenceDeleted))
        observer_->OnNodeChildrenChanged(event.node_id);
    }

    observer_->OnNodeModelChanged(event.node_id);
  });
}

void NodeServiceTreeImpl::OnNodeSemanticChanged(const scada::NodeId& node_id) {
  // TODO: weak_ptr
  Dispatch(executor_, [this, node_id] {
    if (observer_)
      observer_->OnNodeSemanticsChanged(node_id);
  });
}
