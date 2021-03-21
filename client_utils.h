#pragma once

#include "base/promise.h"
#include "core/node_id.h"
#include "node_id_set.h"
#include "node_service/node_ref.h"
#include "window_definition.h"

#include <string>

namespace base {
class FilePath;
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
class TimedDataSpec;

std::wstring GetTimedDataTooltipText(const TimedDataSpec& timed_data);

// TODO: Move to different file.
void ReportRequestResult(const std::wstring& title,
                         const scada::Status& status,
                         LocalEvents& local_events,
                         Profile& profile);

bool ExecuteDisableItem(TaskManager& task_manager,
                        const NodeRef& node,
                        bool disable);

const size_t kTableLimitation = 1000;

promise<NodeIdSet> ExpandGroupItemIds(const NodeRef& node,
                                      size_t max_count = kTableLimitation);

void CompletePath(const std::wstring& text,
                  int& start,
                  std::vector<std::wstring>& list);

void DeleteTreeRecordsRecursive(TaskManager& task_manager, const NodeRef& node);

using NamedNodes = std::vector<std::pair<std::wstring, NodeRef>>;

void SortNamedNodes(NamedNodes& list);

NamedNodes GetNamedNodes(const NodeRef& root,
                         const scada::NodeId& type_definition_id);

std::wstring FormatHostName(const std::string& host_name);

void GetNodesRecursive(const NodeRef& parent, std::vector<NodeRef>& nodes);

void CopyNodesToClipboard(const std::vector<NodeRef>& nodes);
bool PasteNodesFromClipboard(TaskManager& task_manager,
                             const scada::NodeId& new_parent_id);
NodeRef GetPasteParentNode(NodeService& node_service,
                           const NodeRef& selected_node,
                           const NodeRef& root_node);

bool IsWebUrl(std::wstring_view str);
std::wstring MakeFileUrl(const base::FilePath& path);

base::FilePath GetPublicFilePath(const base::FilePath& path);
base::FilePath FullFilePathToPublic(const base::FilePath& path);
