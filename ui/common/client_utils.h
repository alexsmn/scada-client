#pragma once

#include "base/awaitable.h"
#include "base/any_executor.h"
#include "base/promise.h"
#include "controller/node_id_set.h"
#include "node_service/node_ref.h"
#include "scada/node_id.h"

#include <string>

class TaskManager;
class TimedDataSpec;

std::u16string GetTimedDataTooltipText(const TimedDataSpec& timed_data);

const size_t kTableLimitation = 1000;

promise<NodeIdSet> ExpandGroupItemIds(const NodeRef& node,
                                      size_t max_count = kTableLimitation);
Awaitable<NodeIdSet> ExpandGroupItemIdsAsync(
    AnyExecutor executor,
    const NodeRef& node,
    size_t max_count = kTableLimitation);

void CompletePath(const std::u16string& text,
                  int& start,
                  std::vector<std::u16string>& list);

void DeleteTreeRecordsRecursive(TaskManager& task_manager, const NodeRef& node);

using NamedNodes = std::vector<std::pair<std::u16string, NodeRef>>;

void SortNamedNodes(NamedNodes& list);

NamedNodes GetNamedNodes(const NodeRef& root,
                         const scada::NodeId& type_definition_id);

std::u16string FormatHostName(std::string_view host_name);

void GetNodesRecursive(const NodeRef& parent, std::vector<NodeRef>& nodes);
