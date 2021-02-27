#include "components/configuration_tree/node_service_tree_impl.h"

#include "node_service/node_service.h"
#include "node_service/node_util.h"

NodeServiceTreeImpl::NodeServiceTreeImpl(NodeServiceTreeImplContext&& context)
    : NodeServiceTreeImplContext{std::move(context)} {}

NodeRef NodeServiceTreeImpl::GetRoot() const {
  return root_node_;
}

std::vector<NodeServiceTreeImpl::ChildRef> NodeServiceTreeImpl::GetChildren(
    const NodeRef& node) const {
  std::vector<ChildRef> children;

  for (const auto& [reference_type_id, forward] : reference_filter_) {
    const auto& targets = forward ? node.targets(reference_type_id)
                                  : node.inverse_targets(reference_type_id);
    for (const auto& child_node : targets) {
      if (IsMatchingNode(child_node))
        children.emplace_back(ChildRef{reference_type_id, forward, child_node});
    }
  }

  return children;
}

bool NodeServiceTreeImpl::IsMatchingNode(const NodeRef& node) const {
  assert(node);

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
      return false;
  }

  return true;
}
