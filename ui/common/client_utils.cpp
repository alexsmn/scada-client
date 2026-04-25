#include "ui/common/client_utils.h"

#include "aui/translation.h"
#include "base/awaitable_promise.h"
#include "base/executor_conversions.h"
#include "base/format_time.h"
#include "base/u16format.h"
#include "base/utf_convert.h"
#include "base/win/clipboard.h"
#include "ui/common/client_utils.h"
#include "common/formula_util.h"
#include "resources/common_resources.h"
#include "events/event_set.h"
#include "model/data_items_node_ids.h"
#include "model/devices_node_ids.h"
#include "model/node_id_util.h"
#include "node_service/node_promises.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"
#include "profile/profile.h"
#include "events/local_events.h"
#include "services/task_manager.h"
#include "timed_data/timed_data_spec.h"

std::u16string FormatHostName(std::string_view host_name) {
  if (host_name.empty()) {
    return Translate("Local");
  } else {
    return UtfConvert<char16_t>(host_name);
  }
}

inline void AppendHint(std::u16string& hint,
                       std::u16string_view title,
                       std::u16string_view value) {
  if (value.empty())
    return;
  hint += u16format(L"\n{}: {}", title, value);
}

std::u16string GetTimedDataTooltipText(const TimedDataSpec& timed_data) {
  auto name = timed_data.GetTitle();

  auto val = timed_data.GetCurrentString();

  auto str_time = UtfConvert<char16_t>(
      FormatTime(timed_data.change_time(),
                 TIME_FORMAT_DATE | TIME_FORMAT_TIME | TIME_FORMAT_MSEC));
  auto str_utime = UtfConvert<char16_t>(
      FormatTime(timed_data.current().source_timestamp,
                 TIME_FORMAT_DATE | TIME_FORMAT_TIME | TIME_FORMAT_MSEC));

  auto str = name;
  AppendHint(str, Translate("Value"), val);
  AppendHint(str, Translate("Time"), str_time);
  AppendHint(str, Translate("Updated"), str_utime);

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
        str += u16format(L"\n(+ {} {})", events->size() - count,
                         Translate("events"));
        break;
      }
      // add event
      std::u16string stime = UtfConvert<char16_t>(FormatTime(
          event.time, TIME_FORMAT_DATE | TIME_FORMAT_TIME | TIME_FORMAT_MSEC));
      str += u16format(L"\n{} {}", stime, event.message);
    }
  }

  return str;
}

promise<NodeIdSet> ExpandGroupItemIds(const NodeRef& node, size_t max_count) {
  auto executor = MakeThreadAnyExecutor();
  return ToPromise(executor,
                   ExpandGroupItemIdsAsync(executor, node, max_count));
}

namespace {

void InsertLimited(NodeIdSet& target, const NodeIdSet& source, size_t max_count) {
  for (const auto& node_id : source) {
    if (target.size() >= max_count)
      return;
    target.emplace(node_id);
  }
}

}  // namespace

Awaitable<NodeIdSet> ExpandGroupItemIdsAsync(AnyExecutor executor,
                                             const NodeRef& node,
                                             size_t max_count) {
  if (max_count == 0)
    co_return NodeIdSet{};

  co_await AwaitPromise(executor, FetchChildren(node));

  NodeIdSet node_ids;
  if (node.node_class() == scada::NodeClass::Variable) {
    node_ids.emplace(node.node_id());
  }

  for (const auto& child : node.targets(scada::id::Organizes)) {
    if (node_ids.size() >= max_count)
      break;
    auto child_node_ids =
        co_await ExpandGroupItemIdsAsync(executor, child,
                                         max_count - node_ids.size());
    InsertLimited(node_ids, child_node_ids, max_count);
  }

  for (const auto& child : node.targets(scada::id::HasComponent)) {
    if (node_ids.size() >= max_count)
      break;
    auto child_node_ids =
        co_await ExpandGroupItemIdsAsync(executor, child,
                                         max_count - node_ids.size());
    InsertLimited(node_ids, child_node_ids, max_count);
  }

  co_return node_ids;
}

void CompletePath(const std::u16string& text,
                  int& start,
                  std::vector<std::u16string>& list) {}

void DeleteTreeRecordsRecursive(TaskManager& task_manager,
                                const NodeRef& node) {
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
