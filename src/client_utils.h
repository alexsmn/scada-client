#pragma once

#include "base/strings/string16.h"
#include "controls/types.h"
#include "window_definition.h"

namespace rt {
class TimedDataSpec;
}

namespace scada {
class MethodService;
class Status;
}

class ClientApplication;
class DialogService;
class FileCache;
class LocalEvents;
class MainWindow;
class NodeRef;
class NodeRefService;
class OpenedView;
class Page;
class Profile;
class TaskManager;

extern UINT CF_TRECS;

base::string16 GetTimedDataTooltipText(const rt::TimedDataSpec& timed_data);

// TODO: Move into different file.
void ReportRequestResult(const base::string16& title, const scada::Status& status, LocalEvents& local_events, Profile& profile);

const size_t kTableLimitation = 1000;

using WindowDefinitionCallback = std::function<void(WindowDefinition)>;

WindowDefinition PrepareWindowDefinitionForOpen(const NodeRef& node, unsigned type);
void PrepareWindowDefinitionForOpenExpandGroups(const NodeRef& node, unsigned type, const WindowDefinitionCallback& callback);
WindowDefinition PrepareWindowDefinitionForOpen(const NodeRef& node, unsigned type, const std::vector<scada::NodeId>& item_ids);
WindowDefinition PrepareWindowDefinitionForOpen(const char* formula, unsigned type);
WindowDefinition PrepareWindowDefinitionForOpen(const NodeIdSet& items, unsigned type,
    const base::char16* title = nullptr);
void PrepareWindowDefinitionForGroup(const NodeRef& node, unsigned type, const WindowDefinitionCallback& callback);

void ExportConfigurationToExcel(DialogService& dialog_service, NodeRefService& node_service);
void ImportConfigurationFromExcel(DialogService& dialog_service, NodeRefService& node_service, TaskManager& task_manager);

bool ExecuteDisableItem(const NodeRef& node, bool disable, TaskManager& task_manager);

void DoIOCtrl(const scada::NodeId& node_id, const scada::NodeId& method_id,
              const std::vector<scada::Variant>& arguments, LocalEvents& local_events, Profile& profile,
              scada::MethodService& method_service);

using ExpandGroupItemIdsCallback = std::function<void(std::vector<scada::NodeId> nodes)>;
void ExpandGroupItemIds(const NodeRef& node, const ExpandGroupItemIdsCallback& callback);

void CompletePath(const base::string16& text, int& start, std::vector<base::string16>& list);

void DeleteTreeRecordsRecursive(const NodeRef& node, TaskManager& task_manager);

void PrepareDeviceMetricsView(const NodeRef& device, const WindowDefinitionCallback& callback);

int ShowMessageBox(DialogService& dialog_service, const base::char16* message, const base::char16* title, unsigned types);
