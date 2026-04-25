#pragma once

#include "base/awaitable.h"
#include "base/any_executor.h"
#include "base/promise.h"
#include "controller/node_id_set.h"
#include "profile/window_definition.h"

class NodeRef;
struct OpenContext;

promise<WindowDefinition> MakeWindowDefinition(const WindowInfo* window_info,
                                               const NodeRef& node,
                                               bool expand_groups);
Awaitable<WindowDefinition> MakeWindowDefinitionAsync(
    AnyExecutor executor,
    const WindowInfo* window_info,
    NodeRef node,
    bool expand_groups);
promise<WindowDefinition> MakeWindowDefinition(const WindowInfo* window_info,
                                               const OpenContext& open_context);
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

promise<std::optional<WindowDefinition>> MakeGroupWindowDefinition(
    const WindowInfo* window_info,
    const NodeRef& node);
Awaitable<std::optional<WindowDefinition>> MakeGroupWindowDefinitionAsync(
    AnyExecutor executor,
    const WindowInfo* window_info,
    NodeRef node);
