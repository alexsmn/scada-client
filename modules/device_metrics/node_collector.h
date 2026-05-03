#pragma once

#include "base/any_executor.h"
#include "base/awaitable.h"
#include "base/range_util.h"
#include "model/scada_node_ids.h"
#include "node_service/node_ref.h"
#include "node_service/node_util.h"

#include <span>

Awaitable<NodeRef> FetchNodeAsync(AnyExecutor executor,
                                  const NodeRef& node,
                                  const NodeFetchStatus& requested_status);

Awaitable<std::vector<NodeRef>> CollectChildrenAsync(
    AnyExecutor executor,
    const NodeRef& parent_node,
    const scada::NodeId& type_definition_id);

Awaitable<std::vector<NodeRef>> CollectNodesRecursiveAsync(
    AnyExecutor executor,
    const NodeRef& parent_node,
    const scada::NodeId& type_definition_id);

std::set<NodeRef> CollectTypeDefinitions(std::span<const NodeRef> devices);

std::vector<NodeRef> GetSupertypes(NodeRef type_definition);

std::set<NodeRef> CollectVariables(std::span<const NodeRef> devices);
