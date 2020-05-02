#include "client_utils.h"

#include "base/format_time.h"
#include "base/path_service.h"
#include "base/win/clipboard.h"
#include "client_paths.h"
#include "common/event_manager.h"
#include "common/formula_util.h"
#include "common/node_service.h"
#include "common/node_state.h"
#include "common/node_util.h"
#include "common_resources.h"
#include "contents_model.h"
#include "model/data_items_node_ids.h"
#include "model/devices_node_ids.h"
#include "model/node_id_util.h"
#include "model/scada_node_ids.h"
#include "node_serialization.h"
#include "remote/protocol_utils.h"
#include "remote/session_proxy.h"
#include "services/file_cache.h"
#include "services/local_events.h"
#include "services/profile.h"
#include "services/task_manager.h"
#include "timed_data/timed_data_spec.h"
#include "window_info.h"

namespace {

const base::char16 kHttpPrefix[] = L"http://";
const base::char16 kHttpsPrefix[] = L"https://";
const base::char16 kFilePrefix[] = L"file://";

const UINT kNodeTreeHeaderFormat =
    ::RegisterClipboardFormat(L"CE4D311D-FB1C-4972-9EA8-3C2C1FB5091A");
const UINT kNodeTreeFormat =
    ::RegisterClipboardFormat(L"EFCAD60E-2623-4eef-8DE9-9B030DCD3AFE");

}  // namespace

inline void AppendHint(base::string16& hint,
                       const base::char16* title,
                       const base::string16& value) {
  if (value.empty())
    return;
  hint += base::StringPrintf(L"\n%ls: %ls", title, value.c_str());
}

base::string16 GetTimedDataTooltipText(const TimedDataSpec& timed_data) {
  base::string16 name = timed_data.GetTitle();

  base::string16 val = timed_data.GetCurrentString();

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

  // events
  const EventSet* events = timed_data.GetEvents();
  if (events && !events->empty()) {
    size_t count = 0;
    for (EventSet::const_reverse_iterator i = events->rbegin();
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
  if (item_ids.size() >= kTableLimitation)
    return;

  if (node.node_class() == scada::NodeClass::Variable)
    item_ids.insert(node.node_id());

  for (const auto& child : node.targets(scada::id::Organizes))
    ExpandGroupItemIds(child, item_ids);

  for (const auto& child : node.targets(scada::id::HasComponent))
    ExpandGroupItemIds(child, item_ids);
}

WindowDefinition MakeWindowDefinition(const NodeRef& node,
                                      unsigned type,
                                      bool expand_groups) {
  if (!type)
    type = ID_GRAPH_VIEW;

#if defined(UI_QT)
  if (type == ID_PROPERTY_VIEW)
    type = ID_NEW_PROPERTY_VIEW;
#endif

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
  if (!node[devices::id::DeviceType_Disabled])
    return false;

  task_manager.PostUpdateTask(node.node_id(), {},
                              {{devices::id::DeviceType_Disabled, disable}});
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

WindowDefinition MakeWindowDefinition(
    const std::vector<scada::NodeId>& node_ids,
    unsigned type,
    const base::char16* title) {
  if (!type)
    type = ID_TABLE_VIEW;

  WindowDefinition win(GetWindowInfo(type));
  if (title)
    win.title = title;

  for (auto& node_id : node_ids) {
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
  if (!IsInstanceOf(parent, data_items::id::DataGroupType))
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

void GetNodesRecursive(const NodeRef& parent, std::vector<NodeRef>& nodes) {
  for (auto& node : parent.targets(scada::id::Organizes)) {
    nodes.emplace_back(node);
    GetNodesRecursive(node, nodes);
  }
}

std::string ReadClipboard(UINT format) {
  size_t size = 0;
  char* data = nullptr;
  if (!Clipboard().GetData(format, data, size))
    return std::string();

  std::string v(data, data + size);
  delete[] data;
  return v;
}

void CopyNodesToClipboard(const std::vector<NodeRef>& nodes) {
  assert(!nodes.empty());

  Clipboard clipboard;

  {
    protocol::NodeId message;
    ToProto(nodes.front().type_definition().node_id(), message);
    auto buffer = message.SerializePartialAsString();
    if (!clipboard.SetData(kNodeTreeHeaderFormat, buffer.data(), buffer.size()))
      LOG(ERROR) << "Can't set clipboard data";
  }

  {
    std::vector<scada::NodeState> browse_nodes;
    browse_nodes.reserve(nodes.size());

    for (auto& node : nodes) {
      browse_nodes.emplace_back();
      NodeToData(node, browse_nodes.back(), true, true);
    }

    protocol::NodeTree message;
    ContainerToProto(browse_nodes, *message.mutable_node());
    auto buffer = message.SerializePartialAsString();

    if (!clipboard.SetData(kNodeTreeFormat, buffer.data(), buffer.size()))
      LOG(ERROR) << "Can't set clipboard data";
  }
}

void PasteNodesFromClipboardHelper(TaskManager& task_manager,
                                   scada::NodeState&& node_state) {
  task_manager.PostInsertTask(
      {}, node_state.parent_id, node_state.type_definition_id,
      std::move(node_state.attributes), std::move(node_state.properties),
      [&task_manager, references = std::move(node_state.references),
       children = std::move(node_state.children)](
          const scada::Status& status, const scada::NodeId& node_id) mutable {
        if (!status)
          return;

        for (auto& reference : references) {
          if (reference.forward)
            task_manager.PostAddReference(reference.reference_type_id, node_id,
                                          reference.node_id);
        }

        for (auto& child : children) {
          assert(child.reference_type_id == scada::id::Organizes);
          child.parent_id = node_id;
          PasteNodesFromClipboardHelper(task_manager, std::move(child));
        }
      });
}

bool PasteNodesFromClipboard(TaskManager& task_manager,
                             const scada::NodeId& new_parent_id) {
  const auto buffer = ReadClipboard(kNodeTreeFormat);
  if (buffer.empty())
    return false;

  protocol::NodeTree message;
  if (!message.ParseFromString(buffer))
    return false;

  for (const auto& packed_node : message.node()) {
    auto node_state = FromProto(packed_node);
    assert(node_state.reference_type_id == scada::id::Organizes);
    node_state.parent_id = new_parent_id;
    PasteNodesFromClipboardHelper(task_manager, std::move(node_state));
  }

  return true;
}

NodeRef GetPasteParentNode(NodeService& node_service,
                           const NodeRef& selected_node,
                           const NodeRef& root_node) {
  const auto buffer = ReadClipboard(kNodeTreeHeaderFormat);
  if (buffer.empty())
    return false;

  protocol::NodeId message;
  if (!message.ParseFromString(buffer))
    return false;

  const auto& type_definition_id = FromProto(message);
  const auto& type_definition = node_service.GetNode(type_definition_id);
  if (!type_definition)
    return false;

  for (const auto& node : {selected_node, root_node}) {
    for (auto n = node; n; n = n.parent()) {
      if (CanCreate(n, type_definition))
        return n;
    }
  }

  return nullptr;
}

bool IsWebUrl(base::StringPiece16 str) {
  return str.starts_with(kHttpPrefix) || str.starts_with(kHttpsPrefix);
}

base::string16 MakeFileUrl(const base::FilePath& path) {
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
