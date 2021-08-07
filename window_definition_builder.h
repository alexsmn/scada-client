#pragma once

#include "base/promise.h"
#include "node_id_set.h"
#include "window_definition.h"

class NodeRef;
struct OpenContext;

promise<WindowDefinition> MakeWindowDefinition(const WindowInfo* window_info,
                                               const NodeRef& node,
                                               bool expand_groups);
promise<WindowDefinition> MakeWindowDefinition(const WindowInfo* window_info,
                                               const OpenContext& open_context);
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
    std::wstring title = std::wstring{});

promise<std::optional<WindowDefinition>> MakeGroupWindowDefinition(
    const WindowInfo* window_info,
    const NodeRef& node);
