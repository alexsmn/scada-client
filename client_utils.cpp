#include "client_utils.h"

#include "base/format_time.h"
#include "base/path_service.h"
#include "base/range_util.h"
#include "base/win/clipboard.h"
#include "client_paths.h"
#include "common/event_fetcher.h"
#include "common/formula_util.h"
#include "common/node_state.h"
#include "common_resources.h"
#include "contents_model.h"
#include "model/data_items_node_ids.h"
#include "model/devices_node_ids.h"
#include "model/node_id_util.h"
#include "node_service/node_promises.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"
#include "remote/session_proxy.h"
#include "services/file_cache.h"
#include "services/local_events.h"
#include "services/profile.h"
#include "services/task_manager.h"
#include "timed_data/timed_data_spec.h"
#include "window_info.h"

namespace {

const wchar_t kHttpPrefix[] = L"http://";
const wchar_t kHttpsPrefix[] = L"https://";
const wchar_t kFilePrefix[] = L"file://";

}  // namespace

inline void AppendHint(std::wstring& hint,
                       const wchar_t* title,
                       const std::wstring& value) {
  if (value.empty())
    return;
  hint += base::StringPrintf(L"\n%ls: %ls", title, value.c_str());
}

std::wstring GetTimedDataTooltipText(const TimedDataSpec& timed_data) {
  std::wstring name = timed_data.GetTitle();

  std::wstring val = timed_data.GetCurrentString();

  std::wstring str_time = base::SysNativeMBToWide(
      FormatTime(timed_data.change_time(),
                 TIME_FORMAT_DATE | TIME_FORMAT_TIME | TIME_FORMAT_MSEC));
  std::wstring str_utime = base::SysNativeMBToWide(
      FormatTime(timed_data.current().source_timestamp,
                 TIME_FORMAT_DATE | TIME_FORMAT_TIME | TIME_FORMAT_MSEC));

  std::wstring str = name;
  AppendHint(str, L"Значение", val);
  AppendHint(str, L"Время", str_time);
  AppendHint(str, L"Обновлен", str_utime);

  // events
  const auto* events = timed_data.GetEvents();
  if (events && !events->empty()) {
    size_t count = 0;
    for (auto i = events->rbegin(); i != events->rend(); ++i, ++count) {
      const scada::Event& event = **i;
      if (!count)
        str += L'\n';
      // limit for 3 events
      if (count >= 3) {
        str += base::StringPrintf(L"\n(+ %d событий)", events->size() - count);
        break;
      }
      // add event
      std::wstring stime = base::SysNativeMBToWide(FormatTime(
          event.time, TIME_FORMAT_DATE | TIME_FORMAT_TIME | TIME_FORMAT_MSEC));
      str += base::StringPrintf(L"\n%ls %ls", stime.c_str(),
                                event.message.c_str());
    }
  }

  return str;
}

void ReportRequestResult(const std::wstring& title,
                         const scada::Status& status,
                         LocalEvents& local_events,
                         Profile& profile) {
  if (status && !profile.show_write_ok)
    return;

  std::wstring message = base::StringPrintf(L"%ls - %ls.", title.c_str(),
                                            ToString16(status).c_str());
  LocalEvents::Severity severity =
      status ? LocalEvents::SEV_INFO : LocalEvents::SEV_ERROR;
  local_events.ReportEvent(severity, message);
}

/*promise<void> ExpandGroupItemIdsHelper(
    const NodeRef& node,
    size_t max_count,
    const std::shared_ptr<NodeIdSet>& node_ids) {
  return FetchChildren(node).then([node, max_count, node_ids] {
    std::vector<promise<void>> promises;

    if (node.node_class() == scada::NodeClass::Variable)
      node_ids->emplace(node.node_id());

    for (const auto& child : node.targets(scada::id::Organizes)) {
      promises.emplace_back(
          ExpandGroupItemIdsHelper(child, max_count, node_ids));
    }

    for (const auto& child : node.targets(scada::id::HasComponent)) {
      promises.emplace_back(
          ExpandGroupItemIdsHelper(child, max_count, node_ids));
    }

    return make_all_promise_void(std::move(promises));
  });
}

promise<NodeIdSet> ExpandGroupItemIds(const NodeRef& node, size_t max_count) {
  auto node_ids = std::make_shared<NodeIdSet>();
  return ExpandGroupItemIdsHelper(node, max_count, node_ids).then([node_ids] {
    return std::move(*node_ids);
  });
}*/

promise<NodeIdSet> ExpandGroupItemIds(const NodeRef& node, size_t max_count) {
  return FetchChildren(node).then([node, max_count] {
    std::vector<promise<NodeIdSet>> promises;

    if (node.node_class() == scada::NodeClass::Variable) {
      promises.emplace_back(
          make_resolved_promise(MakeNodeIdSet(node.node_id())));
    }

    for (const auto& child : node.targets(scada::id::Organizes))
      promises.emplace_back(ExpandGroupItemIds(child, max_count));

    for (const auto& child : node.targets(scada::id::HasComponent))
      promises.emplace_back(ExpandGroupItemIds(child, max_count));

    return make_all_promise(std::move(promises))
        .then([](const std::vector<NodeIdSet>& subsets) {
          return Union(subsets);
        });
  });
}

bool ExecuteDisableItem(TaskManager& task_manager,
                        const NodeRef& node,
                        bool disable) {
  if (!node[devices::id::DeviceType_Disabled])
    return false;

  task_manager.PostUpdateTask(node.node_id(), {},
                              {{devices::id::DeviceType_Disabled, disable}});
  return true;
}

void CompletePath(const std::wstring& text,
                  int& start,
                  std::vector<std::wstring>& list) {}

void DeleteTreeRecordsRecursive(TaskManager& task_manager,
                                const NodeRef& node) {
  /*for (auto* child : scada::GetComponents(node))
    DeleteTreeRecordsRecursive(*child);*/

  task_manager.PostDeleteTask(node.node_id());
}

void SortNamedNodes(NamedNodes& list) {
  std::sort(list.begin(), list.end(),
            [](const auto& a, const auto& b) { return a.first < b.first; });
}

void GetNamedNodesHelper(const NodeRef& parent,
                         const scada::NodeId& type_definition_id,
                         NamedNodes& result) {
  for (const auto& child : parent.targets(scada::id::Organizes)) {
    if (IsInstanceOf(child, type_definition_id)) {
      result.emplace_back(GetFullDisplayName(child), child);
      GetNamedNodesHelper(child, type_definition_id, result);
    }
  }
}

NamedNodes GetNamedNodes(const NodeRef& root,
                         const scada::NodeId& type_definition_id) {
  NamedNodes result;
  result.reserve(32);
  GetNamedNodesHelper(root, type_definition_id, result);
  return result;
}

void GetNodesRecursive(const NodeRef& parent, std::vector<NodeRef>& nodes) {
  for (auto& node : parent.targets(scada::id::Organizes)) {
    nodes.emplace_back(node);
    GetNodesRecursive(node, nodes);
  }
}

bool IsWebUrl(std::wstring_view str) {
  return base::StartsWith(ToStringPiece(str), kHttpPrefix,
                          base::CompareCase::SENSITIVE) ||
         base::StartsWith(ToStringPiece(str), kHttpsPrefix,
                          base::CompareCase::SENSITIVE);
}

std::wstring MakeFileUrl(const base::FilePath& path) {
  return kFilePrefix + path.AsUTF16Unsafe();
}

base::FilePath GetPublicFilePath(const base::FilePath& path) {
  base::FilePath public_path;
  base::PathService::Get(client::DIR_PUBLIC, &public_path);
  return public_path.Append(path);
}

base::FilePath FullFilePathToPublic(const base::FilePath& path) {
  return path.BaseName();
}
