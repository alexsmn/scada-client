#include "client_utils.h"

#include "base/format_time.h"
#include "common/event_manager.h"
#include "common/formula_util.h"
#include "common/node_id_util.h"
#include "common/node_service.h"
#include "common/node_util.h"
#include "common/scada_node_ids.h"
#include "common_resources.h"
#include "contents_model.h"
#include "remote/session_proxy.h"
#include "services/file_cache.h"
#include "services/local_events.h"
#include "services/profile.h"
#include "services/task_manager.h"
#include "timed_data/timed_data_spec.h"
#include "window_info.h"

inline void AppendHint(base::string16& hint,
                       const base::char16* title,
                       const base::string16& value) {
  if (value.empty())
    return;
  hint += base::StringPrintf(L"\n%ls: %ls", title, value.c_str());
}

base::string16 GetTimedDataTooltipText(const rt::TimedDataSpec& timed_data) {
  base::string16 name = timed_data.GetTitle();

  base::string16 val = timed_data.GetCurrentString();
  // TODO: Read user.
  /*if (item.value_user) {
    const scada::User* user = static_cast<const scada::User*>(
      monitored_item_service.table(NamespaceIndexes::USER).FindRecord(item.value_user));
    if (user)
      val += base::StringPrintf(" (%s)", user->name);
  }*/

  // source
  base::string16 source;

  /*scada::Node* node = monitored_item_service.object_tree().FindNode(tbl.id,
  item.id); bool simulated = node && node->IsSimulated(true);

  if (simulated) {
    std::string sim_item_name = monitored_item_service.GetNodeTitle(
      NamespaceIndexes::SIM_ITEM, item.simulation_signal);
    source = "Эмулятор - " + sim_item_name;

  } else {
    const scada::Item::Channel* chan = item.get_channel();
    if (chan) {
      std::string device_name = monitored_item_service.
        GetDeviceName(chan->device_type, chan->device_id);
      source = base::StringPrintf("%s : %s", device_name.c_str(),
  chan->item_path);
    }
  }*/

  base::string16 str_time = base::SysNativeMBToWide(
      FormatTime(timed_data.change_time(),
                 TIME_FORMAT_DATE | TIME_FORMAT_TIME | TIME_FORMAT_MSEC));
  base::string16 str_utime = base::SysNativeMBToWide(
      FormatTime(timed_data.current().source_timestamp,
                 TIME_FORMAT_DATE | TIME_FORMAT_TIME | TIME_FORMAT_MSEC));

  base::string16 str = name;
  AppendHint(str, L"Значение", val);
  AppendHint(str, L"Время", str_time);
  AppendHint(str, L"Обновлен", str_utime);
  AppendHint(str, L"Источник", source);

  // events
  const events::EventSet* events = timed_data.GetEvents();
  if (events && !events->empty()) {
    size_t count = 0;
    for (events::EventSet::const_reverse_iterator i = events->rbegin();
         i != events->rend(); ++i, ++count) {
      const scada::Event& event = **i;
      if (!count)
        str += L'\n';
      // limit for 3 events
      if (count >= 3) {
        str += base::StringPrintf(L"\n(+ %d событий)", events->size() - count);
        break;
      }
      // add event
      base::string16 stime = base::SysNativeMBToWide(FormatTime(
          event.time, TIME_FORMAT_DATE | TIME_FORMAT_TIME | TIME_FORMAT_MSEC));
      str += base::StringPrintf(L"\n%ls %ls", stime.c_str(),
                                event.message.c_str());
    }
  }

  return str;
}

void ReportRequestResult(const base::string16& title,
                         const scada::Status& status,
                         LocalEvents& local_events,
                         Profile& profile) {
  if (status && !profile.show_write_ok)
    return;

  base::string16 message = base::StringPrintf(L"%ls - %ls.", title.c_str(),
                                              ToString16(status).c_str());
  LocalEvents::Severity severity =
      status ? LocalEvents::SEV_INFO : LocalEvents::SEV_ERROR;
  local_events.ReportEvent(severity, message);
}

void ExpandGroupItemIds(const NodeRef& node, NodeIdSet& item_ids) {
  for (const auto& child : node.targets(scada::id::Organizes)) {
    if (child.node_class() == scada::NodeClass::Variable)
      item_ids.insert(child.node_id());

    ExpandGroupItemIds(child, item_ids);

    if (item_ids.size() >= kTableLimitation)
      break;
  }
}

WindowDefinition MakeWindowDefinition(const NodeRef& node,
                                      unsigned type,
                                      bool expand_groups) {
  if (!type)
    type = ID_GRAPH_VIEW;

  NodeIdSet item_ids;
  if (expand_groups && node && node.node_class() == scada::NodeClass::Object)
    ExpandGroupItemIds(node, item_ids);
  else
    item_ids.insert(node.node_id());

  const WindowInfo& window_info = GetWindowInfo(type);
  WindowDefinition win(window_info);
  win.title = base::StringPrintf(L"%ls: %ls", window_info.title,
                                 ToString16(node.display_name()).c_str());

  for (auto& id : item_ids) {
    WindowItem& item_id = win.AddItem("Item");
    item_id.SetString("path", MakeNodeIdFormula(id));
  }

  return win;
}

WindowDefinition MakeWindowDefinition(const NodeRef& node,
                                      unsigned type,
                                      const NodeIdSet& item_ids) {
  if (!type)
    type = ID_GRAPH_VIEW;

  const WindowInfo& window_info = GetWindowInfo(type);
  WindowDefinition win(GetWindowInfo(type));
  win.title = base::StringPrintf(L"%ls: %ls", window_info.title,
                                 ToString16(node.display_name()).c_str());

  for (auto& id : item_ids) {
    WindowItem& item_id = win.AddItem("Item");
    item_id.SetString("path", MakeNodeIdFormula(id));
  }

  return win;
}

bool ExecuteDisableItem(TaskManager& task_manager,
                        const NodeRef& node,
                        bool disable) {
  if (!node[id::DeviceType_Disabled])
    return false;

  task_manager.PostUpdateTask(node.node_id(), {},
                              {{id::DeviceType_Disabled, disable}});
  return true;
}

void CompletePath(const base::string16& text,
                  int& start,
                  std::vector<base::string16>& list) {}

void DeleteTreeRecordsRecursive(TaskManager& task_manager,
                                const NodeRef& node) {
  /*for (auto* child : scada::GetComponents(node))
    DeleteTreeRecordsRecursive(*child);*/

  task_manager.PostDeleteTask(node.node_id());
}

WindowDefinition MakeWindowDefinition(const NodeIdSet& items,
                                      unsigned type,
                                      const base::char16* title) {
  if (!type)
    type = ID_TABLE_VIEW;

  WindowDefinition win(GetWindowInfo(type));
  if (title)
    win.title = title;

  for (auto& node_id : items) {
    WindowItem& item = win.AddItem("Item");
    item.SetString("path", NodeIdToScadaString(node_id));
  }

  return win;
}

WindowDefinition MakeWindowDefinition(const char* formula, unsigned type) {
  if (!type)
    type = ID_GRAPH_VIEW;

  WindowDefinition win(GetWindowInfo(type));
  win.title = base::SysNativeMBToWide(formula);

  WindowItem& item = win.AddItem("Item");
  item.SetString("path", formula);

  return win;
}

std::optional<WindowDefinition> MakeGroupWindowDefinition(const NodeRef& node,
                                                          unsigned type) {
  auto parent = node.parent();
  if (!IsInstanceOf(parent, id::DataGroupType))
    return std::nullopt;

  NodeIdSet item_ids;
  ExpandGroupItemIds(parent, item_ids);

  return MakeWindowDefinition(node, type, item_ids);
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
