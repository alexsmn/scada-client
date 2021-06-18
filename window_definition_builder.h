#pragma once

#include "base/promise.h"
#include "node_id_set.h"
#include "window_definition.h"

class NodeRef;
struct OpenContext;

promise<WindowDefinition> MakeWindowDefinition(const NodeRef& node,
                                               unsigned type,
                                               bool expand_groups);
promise<WindowDefinition> MakeWindowDefinition(const OpenContext& open_context,
                                               unsigned type);
WindowDefinition MakeSingleWindowDefinition(const NodeRef& node, unsigned type);
WindowDefinition MakeWindowDefinition(const NodeRef& node,
                                      unsigned type,
                                      const NodeIdSet& item_ids);
WindowDefinition MakeWindowDefinition(const char* formula, unsigned type);
WindowDefinition MakeWindowDefinition(
    const std::vector<scada::NodeId>& node_ids,
    unsigned type,
    const wchar_t* title = nullptr);

promise<std::optional<WindowDefinition>> MakeGroupWindowDefinition(
    const NodeRef& node,
    unsigned type);

std::optional<WindowDefinition> MakeDeviceMetricsWindowDefinition(
    const NodeRef& device);
