#pragma once

#include "base/awaitable.h"
#include "base/any_executor.h"
#include "controller/node_id_set.h"
#include "profile/window_definition.h"

class NodeRef;
struct OpenContext;

Awaitable<WindowDefinition> MakeWindowDefinitionAsync(
    AnyExecutor executor,
    const WindowInfo* window_info,
    NodeRef node,
    bool expand_groups);
Awaitable<WindowDefinition> MakeWindowDefinitionAsync(
    AnyExecutor executor,
    const WindowInfo* window_info,
    OpenContext open_context);
WindowDefinition MakeSingleWindowDefinition(const WindowInfo* window_info,
                                            const NodeRef& node);
WindowDefinition MakeWindowDefinition(const WindowInfo* window_info,
                                      const NodeRef& node,
                                      const NodeIdSet& item_ids);
WindowDefinition MakeWindowDefinition(const WindowInfo* window_info,
                                      std::string formula);
WindowDefinition MakeWindowDefinition(
    const WindowInfo* window_info,
    const std::vector<scada::NodeId>& node_ids,
    std::u16string title = std::u16string{});

Awaitable<std::optional<WindowDefinition>> MakeGroupWindowDefinitionAsync(
    AnyExecutor executor,
    const WindowInfo* window_info,
    NodeRef node);
