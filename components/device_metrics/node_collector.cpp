#include "components/device_metrics/node_collector.h"

#include "base/span_util.h"

#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/adaptor/transformed.hpp>

namespace {

std::vector<NodeRef> JoinAll(
    const NodeRef& parent_node,
    std::span<const std::vector<NodeRef>> recursive_children) {
  std::vector<NodeRef> result;
  result.push_back(parent_node);
  for (auto& part : recursive_children) {
    result.insert(result.end(), std::make_move_iterator(part.begin()),
                  std::make_move_iterator(part.end()));
  }
  return result;
}

}  // namespace

promise<NodeRef> FetchNode(const NodeRef& node,
                           const NodeFetchStatus& requested_status) {
  auto promise = make_promise<NodeRef>();
  node.Fetch(requested_status,
             [promise](const NodeRef& node) mutable { promise.resolve(node); });
  return promise;
}

promise<std::vector<NodeRef>> CollectChildren(
    const NodeRef& parent_node,
    const scada::NodeId& type_definition_id) {
  return FetchNode(parent_node, NodeFetchStatus::ChildrenOnly())
      .then([](const NodeRef& fetched_node) {
        return make_all_promise(
            fetched_node.targets(scada::id::Organizes) |
            boost::adaptors::transformed([](const NodeRef& node) {
              return FetchNode(node, NodeFetchStatus::NodeOnly());
            }));
      })
      .then([type_definition_id](const std::vector<NodeRef>& fetched_children) {
        return fetched_children |
               boost::adaptors::filtered(
                   [type_definition_id](const NodeRef& node) {
                     return IsInstanceOf(node, type_definition_id);
                   }) |
               to_vector;
      });
}

promise<std::vector<NodeRef>> CollectNodesRecursive(
    const NodeRef& parent_node,
    const scada::NodeId& type_definition_id) {
  return CollectChildren(parent_node, type_definition_id)
      .then([type_definition_id](const std::vector<NodeRef>& children) {
        return make_all_promise(
            children | boost::adaptors::transformed([type_definition_id](
                                                        const NodeRef& node) {
              return CollectNodesRecursive(node, type_definition_id);
            }));
      })
      .then([parent_node](
                const std::vector<std::vector<NodeRef>>& recursive_children) {
        return JoinAll(parent_node, recursive_children);
      });
}

std::set<NodeRef> CollectTypeDefinitions(std::span<const NodeRef> devices) {
  return AsBaseSpan(devices) |
         boost::adaptors::transformed(std::mem_fn(&NodeRef::type_definition)) |
         to_set;
}

std::vector<NodeRef> GetSupertypes(NodeRef type_definition) {
  std::vector<NodeRef> supertypes;
  for (; type_definition; type_definition = type_definition.supertype())
    supertypes.push_back(type_definition);
  return supertypes;
}

auto GetDataVariableDecls(const NodeRef& type_defintion) {
  return GetSupertypes(type_defintion) |
         boost::adaptors::transformed(&GetDataVariables) | flattened;
}

std::set<NodeRef> CollectVariables(std::span<const NodeRef> devices) {
  return CollectTypeDefinitions(devices) |
         boost::adaptors::transformed(&GetDataVariableDecls) | flattened |
         to_set;
}
