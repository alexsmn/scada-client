#pragma once

#include "base/promise.h"
#include "base/range_util.h"
#include "model/scada_node_ids.h"
#include "node_service/node_ref.h"
#include "node_service/node_util.h"

inline promise<NodeRef> FetchNode(const NodeRef& node,
                                  const NodeFetchStatus& requested_status);

promise<std::vector<NodeRef>> CollectChildren(
    const NodeRef& parent_node,
    const scada::NodeId& type_definition_id);

promise<std::vector<NodeRef>> CollectNodesRecursive(
    const NodeRef& parent_node,
    const scada::NodeId& type_definition_id);

std::set<NodeRef> CollectTypeDefinitions(base::span<const NodeRef> devices);

std::vector<NodeRef> GetSupertypes(NodeRef type_definition);

std::set<NodeRef> CollectVariables(base::span<const NodeRef> devices);
