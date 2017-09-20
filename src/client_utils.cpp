#include "client_utils.h"

#include "base/format_time.h"
#include "common_resources.h"
#include "services/local_events.h"
#include "services/profile.h"
#include "services/task_manager.h"
#include "window_info.h"
#include "common/scada_node_ids.h"
#include "timed_data/timed_data_spec.h"
#include "core/method_service.h"
#include "common/node_ref_service.h"
#include "common/node_ref_util.h"
#include "common/formula_util.h"
#include "common/browse_util.h"

UINT CF_TRECS = RegisterClipboardFormat(L"EFCAD60E-2623-4eef-8DE9-9B030DCD3AFE");

inline void AppendHint(base::string16& hint, const base::char16* title, const base::string16& value) {
  if (value.empty())
    return;
  hint += base::StringPrintf(L"\n%ls: %ls", title, value.c_str());
}

base::string16 GetTimedDataTooltipText(const rt::TimedDataSpec& timed_data) {
  base::string16 name = timed_data.GetTitle();

  base::string16 val = timed_data.GetCurrentString();

  // source
  base::string16 source;

  base::string16 str_time = base::SysNativeMBToWide(FormatTime(timed_data.change_time(),
      TIME_FORMAT_DATE | TIME_FORMAT_TIME | TIME_FORMAT_MSEC));
  base::string16 str_utime = base::SysNativeMBToWide(FormatTime(timed_data.current().source_timestamp,
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
      base::string16 stime = base::SysNativeMBToWide(FormatTime(event.time,
          TIME_FORMAT_DATE | TIME_FORMAT_TIME | TIME_FORMAT_MSEC));
      str += base::StringPrintf(L"\n%ls %ls", stime.c_str(), event.message.c_str());
    }
  }

  return str;
}

void ReportRequestResult(const base::string16& title, const scada::Status& status, LocalEvents& local_events, Profile& profile) {
  if (status && !profile.show_write_ok)
    return;

  base::string16 message = base::StringPrintf(L"%ls - %ls.", title.c_str(),
      status.ToString16().c_str());
  LocalEvents::Severity severity = status ?
      LocalEvents::SEV_INFO : LocalEvents::SEV_ERROR;
  local_events.ReportEvent(severity, message);
}

template<class Callback>
struct ExpandGroupItemIdsHelper : public std::enable_shared_from_this<ExpandGroupItemIdsHelper<Callback>> {
  ExpandGroupItemIdsHelper(NodeRefService& node_service, const Callback& callback)
      : node_service_{node_service},
        callback_{callback} {
  }

  ~ExpandGroupItemIdsHelper() {
    callback_{std::move(item_ids_)};
  }

  bool Traverse(const NodeRef& node) {
    if (item_ids_.size() >= kTableLimitation)
      return false;

    if (node.node_class() == scada::NodeClass::Variable)
      item_ids_.emplace_back(node.id());

    auto self = shared_from_this();
    BrowseNodes(node_service_, {node.id(), scada::BrowseDirection::Forward, OpcUaId_Organizes, true},
        [self](const scada::Status& status, std::vector<NodeRef> nodes) {
          for (auto& node : nodes) {
            if (!self->Traverse(node))
              break;
          }
        });

    return true;
  }

  NodeRefService& node_service_;
  std::vector<scada::NodeId> item_ids_;
  Callback callback_;
};

// Callback = void(std::vector<scada::NodeId>)
void ExpandGroupItemIds(NodeRefService& node_service, const NodeRef& node, const ExpandGroupItemIdsCallback& callback) {
  if (!node) {
    callback({});
    return;
  }

  std::make_shared<ExpandGroupItemIdsHelper<ExpandGroupItemIdsCallback>>(node_service, callback)->Traverse(node);
}

WindowDefinition PrepareWindowDefinitionForOpen(const NodeRef& node, unsigned type) {
  if (!type)
    type = ID_GRAPH_VIEW;

  const WindowInfo& window_info = GetWindowInfo(type);
  WindowDefinition win(window_info);
  win.title = base::StringPrintf(L"%ls: %ls", window_info.title, node.display_name().text().c_str());

  WindowItem& item_id = win.AddItem("Item");
  item_id.SetString("path", MakeNodeIdFormula(node.id()));

  return win;
}

void PrepareWindowDefinitionForOpenExpandGroups(NodeRefService& node_service, const NodeRef& node, unsigned type, const WindowDefinitionCallback& callback) {
  if (!type)
    type = ID_GRAPH_VIEW;

  ExpandGroupItemIds(node_service, node, [node, type, callback](const std::vector<scada::NodeId>& node_ids) {
    const WindowInfo& window_info = GetWindowInfo(type);
    WindowDefinition win(window_info);
    win.title = base::StringPrintf(L"%ls: %ls", window_info.title, node.display_name().text().c_str());

    for (auto& node_id : node_ids) {
      WindowItem& item_id = win.AddItem("Item");
      item_id.SetString("path", MakeNodeIdFormula(node_id));
    }

    callback(std::move(win));
  });
}

WindowDefinition PrepareWindowDefinitionForOpen(const NodeRef& node, unsigned type, const std::vector<scada::NodeId>& item_ids) {
  if (!type)
    type = ID_GRAPH_VIEW;

  const WindowInfo& window_info = GetWindowInfo(type);
  WindowDefinition win(GetWindowInfo(type));
  win.title = base::StringPrintf(L"%ls: %ls", window_info.title, node.display_name().text().c_str());

  for (auto& id : item_ids) {
    WindowItem& item_id = win.AddItem("Item");
    item_id.SetString("path", MakeNodeIdFormula(id));
  }

  return win;
}

bool ExecuteDisableItem(const NodeRef& node, bool disable, TaskManager& task_manager) {
  auto type_defintion = node.type_definition();
  if (!type_defintion || !type_defintion[id::DeviceType_Disabled])
    return false;

  task_manager.PostUpdateTask(node.id(), {}, {{id::DeviceType_Disabled, disable}});
  return true;
}

void DoIOCtrl(const scada::NodeId& node_id, const scada::NodeId& method_id,
              const std::vector<scada::Variant>& arguments, LocalEvents& local_events, Profile& profile,
              scada::MethodService& method_service) {
  method_service.Call(node_id, method_id, arguments,
      [node_id, &local_events, &profile](const scada::Status& status) {
        // TODO: Fill |title|.
        base::string16 title = base::SysNativeMBToWide(node_id.ToString());
        ReportRequestResult(title, status, local_events, profile);
      });
}

void CompletePath(const base::string16& text, int& start, std::vector<base::string16>& list) {
}

void DeleteTreeRecordsRecursive(const NodeRef& node, TaskManager& task_manager) {
  /*for (auto* child : scada::GetOrganizes(node))
    DeleteTreeRecordsRecursive(*child);*/

  task_manager.PostDeleteTask(node.id());
}

WindowDefinition PrepareWindowDefinitionForOpen(const NodeIdSet& items, unsigned type,
    const base::char16* title) {
  if (!type)
    type = ID_TABLE_VIEW;

  WindowDefinition win(GetWindowInfo(type));
  if (title)
    win.title = title;
    
  for (std::set<scada::NodeId>::const_iterator i = items.begin(); i != items.end(); ++i) {
    WindowItem& item = win.AddItem("Item");
    item.SetString("path", i->ToString());
  }

  return win;
}

WindowDefinition PrepareWindowDefinitionForOpen(const char* formula, unsigned type) {
  if (!type)
    type = ID_GRAPH_VIEW;

  WindowDefinition win(GetWindowInfo(type));
  win.title = base::SysNativeMBToWide(formula);

  WindowItem& item = win.AddItem("Item");
  item.SetString("path", formula);

  return win;
}

void PrepareWindowDefinitionForGroup(NodeRefService& node_service, const NodeRef& node, unsigned type, const WindowDefinitionCallback& callback) {
  if (!node) {
    callback({});
    return;
  }

  BrowseParent(node_service, node.id(), OpcUaId_HierarchicalReferences,
      [&node_service, type, callback](const scada::Status& status, const NodeRef& parent) {
        if (!status) {
          callback({});
          return;
        }
        ExpandGroupItemIds(node_service, parent, [&node_service, parent, type, callback](const std::vector<scada::NodeId>& node_ids) {
          callback(PrepareWindowDefinitionForOpen(parent, type, node_ids));
        });
      });
}
