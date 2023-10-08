#pragma once

#include "base/promise.h"
#include "scada/node_id.h"
#include "node_id_set.h"
#include "node_service/node_ref.h"
#include "controller/window_definition.h"

#include <string>

namespace base {
class FilePath;
}

namespace scada {
class Status;
}  // namespace scada

class CreateTree;
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

std::u16string GetTimedDataTooltipText(const TimedDataSpec& timed_data);

// TODO: Move to different file.
void ReportRequestResult(const std::u16string& title,
                         const scada::Status& status,
                         LocalEvents& local_events,
                         Profile& profile);

bool ExecuteDisableItem(TaskManager& task_manager,
                        const NodeRef& node,
                        bool disable);

const size_t kTableLimitation = 1000;

promise<NodeIdSet> ExpandGroupItemIds(const NodeRef& node,
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

NodeRef GetPasteParentNode(NodeService& node_service,
                           CreateTree& create_tree,
                           const NodeRef& selected_node,
                           const NodeRef& root_node);

bool IsWebUrl(std::u16string_view str);
std::u16string MakeFileUrl(const std::filesystem::path& path);
