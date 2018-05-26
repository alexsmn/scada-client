#pragma once

#include "base/strings/string16.h"
#include "common/node_ref.h"
#include "controls/types.h"
#include "core/node_id.h"
#include "window_definition.h"

namespace base {
class FilePath;
}

namespace rt {
class TimedDataSpec;
}

namespace scada {
class Status;
}  // namespace scada

class DialogService;
class FileCache;
class LocalEvents;
class MainWindow;
class NodeService;
class Page;
class Profile;
class OpenedView;
class TaskManager;

base::string16 GetTimedDataTooltipText(const rt::TimedDataSpec& timed_data);

// TODO: Move to different file.
void ReportRequestResult(const base::string16& title,
                         const scada::Status& status,
                         LocalEvents& local_events,
                         Profile& profile);

const size_t kTableLimitation = 1000;

WindowDefinition MakeWindowDefinition(const NodeRef& node,
                                      unsigned type,
                                      bool expand_groups);
WindowDefinition MakeWindowDefinition(const NodeRef& node,
                                      unsigned type,
                                      const NodeIdSet& item_ids);
WindowDefinition MakeWindowDefinition(const char* formula, unsigned type);
WindowDefinition MakeWindowDefinition(const NodeIdSet& items,
                                      unsigned type,
                                      const base::char16* title = nullptr);

std::optional<WindowDefinition> MakeGroupWindowDefinition(const NodeRef& node,
                                                          unsigned type);

std::optional<WindowDefinition> MakeDeviceMetricsWindowDefinition(
    const NodeRef& device);

bool ExecuteDisableItem(TaskManager& task_manager,
                        const NodeRef& node,
                        bool disable);

void ExpandGroupItemIds(const NodeRef& node, NodeIdSet& item_ids);

void CompletePath(const base::string16& text,
                  int& start,
                  std::vector<base::string16>& list);

void DeleteTreeRecordsRecursive(TaskManager& task_manager, const NodeRef& node);

using NamedNodes = std::vector<std::pair<base::string16, NodeRef>>;

void SortNamedNodes(NamedNodes& list);

NamedNodes GetNamedNodes(const NodeRef& root,
                         const scada::NodeId& type_definition_id);

base::string16 FormatHostName(const std::string& host_name);

void GetNodesRecursive(const NodeRef& parent, std::vector<NodeRef>& nodes);

void CopyNodesToClipboard(const std::vector<NodeRef>& nodes);
bool PasteNodesFromClipboard(TaskManager& task_manager,
                             const scada::NodeId& new_parent_id);
NodeRef GetPasteParentNode(NodeService& node_service,
                           const NodeRef& selected_node,
                           const NodeRef& root_node);
