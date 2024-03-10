#include "client_utils.h"

#include "aui/translation.h"
#include "base/format_time.h"
#include "base/range_util.h"
#include "base/string_piece_util.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/win/clipboard.h"
#include "client_utils.h"
#include "common/formula_util.h"
#include "common_resources.h"
#include "events/event_set.h"
#include "model/data_items_node_ids.h"
#include "model/devices_node_ids.h"
#include "model/node_id_util.h"
#include "node_service/node_promises.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"
#include "profile/profile.h"
#include "services/local_events.h"
#include "services/task_manager.h"
#include "timed_data/timed_data_spec.h"

std::u16string FormatHostName(std::string_view host_name) {
  if (host_name.empty()) {
    return Translate("Local");
  } else {
    return base::UTF8ToUTF16(AsStringPiece(host_name));
  }
}

inline void AppendHint(std::u16string& hint,
                       std::u16string_view title,
                       std::u16string_view value) {
  if (value.empty())
    return;
  hint +=
      base::StrCat({u"\n", AsStringPiece(title), u": ", AsStringPiece(value)});
}

std::u16string GetTimedDataTooltipText(const TimedDataSpec& timed_data) {
  auto name = timed_data.GetTitle();

  auto val = timed_data.GetCurrentString();

  auto str_time = base::UTF8ToUTF16(
      FormatTime(timed_data.change_time(),
                 TIME_FORMAT_DATE | TIME_FORMAT_TIME | TIME_FORMAT_MSEC));
  auto str_utime = base::UTF8ToUTF16(
      FormatTime(timed_data.current().source_timestamp,
                 TIME_FORMAT_DATE | TIME_FORMAT_TIME | TIME_FORMAT_MSEC));

  auto str = name;
  AppendHint(str, u"Значение", val);
  AppendHint(str, u"Время", str_time);
  AppendHint(str, u"Обновлен", str_utime);

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
        str += base::StringPrintf(u"\n(+ %d событий)", events->size() - count);
        break;
      }
      // add event
      std::u16string stime = base::UTF8ToUTF16(FormatTime(
          event.time, TIME_FORMAT_DATE | TIME_FORMAT_TIME | TIME_FORMAT_MSEC));
      str += base::StringPrintf(u"\n%ls %ls", stime.c_str(),
                                event.message.c_str());
    }
  }

  return str;
}

void ReportRequestResult(const std::u16string& title,
                         const scada::Status& status,
                         LocalEvents& local_events,
                         Profile& profile) {
  if (status && !profile.show_write_ok)
    return;

  scada::LocalizedText message = base::StringPrintf(
      u"%ls - %ls.", title.c_str(), ToString16(status).c_str());
  LocalEvents::Severity severity =
      status ? LocalEvents::SEV_INFO : LocalEvents::SEV_ERROR;
  local_events.ReportEvent(severity, message);
}

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
