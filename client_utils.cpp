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
#include "node_serialization.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"
#include "remote/protocol_utils.h"
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

const UINT kNodeTreeHeaderFormat =
    ::RegisterClipboardFormat(L"CE4D311D-FB1C-4972-9EA8-3C2C1FB5091A");
const UINT kNodeTreeFormat =
    ::RegisterClipboardFormat(L"EFCAD60E-2623-4eef-8DE9-9B030DCD3AFE");

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

promise<void> FetchChildren(const NodeRef& node) {
  if (node.children_fetched())
    return make_resolved_promise();

  promise<void> promise;
  node.Fetch(NodeFetchStatus::NodeAndChildren(),
             [promise](const NodeRef& node) mutable { promise.resolve(); });
  return promise;
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

WindowDefinition MakeEmptyWindowDefinition(const NodeRef& node, unsigned type) {
  if (!type)
    type = ID_GRAPH_VIEW;

#if defined(UI_QT)
  if (type == ID_PROPERTY_VIEW)
    type = ID_NEW_PROPERTY_VIEW;
#endif

  const WindowInfo& window_info = GetWindowInfo(type);

  WindowDefinition window_def{window_info};
  window_def.title = base::StringPrintf(
      L"%ls: %ls", window_info.title, ToString16(node.display_name()).c_str());
  return window_def;
}

WindowDefinition MakeSingleWindowDefinition(const NodeRef& node,
                                            unsigned type) {
  auto window_def = MakeEmptyWindowDefinition(node, type);

  auto& item_id = window_def.AddItem("Item");
  item_id.SetString("path", MakeNodeIdFormula(node.node_id()));

  return window_def;
}

void AddNodeIds(WindowDefinition& window_def, const NodeIdSet& node_ids) {
  for (const auto& node_id : node_ids) {
    WindowItem& item_id = window_def.AddItem("Item");
    item_id.SetString("path", MakeNodeIdFormula(node_id));
  }
}

// TODO: Combine with |MakeSingleWindowDefinition()|.
promise<WindowDefinition> MakeWindowDefinition(const NodeRef& node,
                                               unsigned type,
                                               bool expand_groups) {
  promise<NodeIdSet> node_ids_promise;
  if (expand_groups && node && node.node_class() == scada::NodeClass::Object)
    node_ids_promise = ExpandGroupItemIds(node);
  else
    node_ids_promise = make_resolved_promise(MakeNodeIdSet(node.node_id()));

  return node_ids_promise.then([node, type](const NodeIdSet& node_ids) {
    auto window_def = MakeEmptyWindowDefinition(node, type);
    AddNodeIds(window_def, node_ids);
    return window_def;
  });
}

WindowDefinition MakeWindowDefinition(const NodeRef& node,
                                      unsigned type,
                                      const NodeIdSet& item_ids) {
  if (!type)
    type = ID_GRAPH_VIEW;

  const WindowInfo& window_info = GetWindowInfo(type);
  WindowDefinition window_def(GetWindowInfo(type));
  window_def.title = base::StringPrintf(
      L"%ls: %ls", window_info.title, ToString16(node.display_name()).c_str());

  for (auto& id : item_ids) {
    WindowItem& item_id = window_def.AddItem("Item");
    item_id.SetString("path", MakeNodeIdFormula(id));
  }

  return window_def;
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

WindowDefinition MakeWindowDefinition(
    const std::vector<scada::NodeId>& node_ids,
    unsigned type,
    const wchar_t* title) {
  if (!type)
    type = ID_TABLE_VIEW;

  WindowDefinition window_def(GetWindowInfo(type));
  if (title)
    window_def.title = title;

  for (auto& node_id : node_ids) {
    WindowItem& item = window_def.AddItem("Item");
    item.SetString("path", NodeIdToScadaString(node_id));
  }

  return window_def;
}

WindowDefinition MakeWindowDefinition(const char* formula, unsigned type) {
  if (!type)
    type = ID_GRAPH_VIEW;

  WindowDefinition window_def(GetWindowInfo(type));
  window_def.title = base::SysNativeMBToWide(formula);

  WindowItem& item = window_def.AddItem("Item");
  item.SetString("path", formula);

  return window_def;
}

promise<std::optional<WindowDefinition>> MakeGroupWindowDefinition(
    const NodeRef& node,
    unsigned type) {
  auto parent = node.parent();
  if (!IsInstanceOf(parent, data_items::id::DataGroupType))
    return make_resolved_promise(std::optional<WindowDefinition>());

  return ExpandGroupItemIds(parent).then(
      [node, type](const NodeIdSet& node_ids) {
        return std::optional<WindowDefinition>{
            MakeWindowDefinition(node, type, node_ids)};
      });
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
    Convert(nodes.front().type_definition().node_id(), message);
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
    Convert(browse_nodes, *message.mutable_node());
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
          if (reference.forward) {
            task_manager.PostAddReference(reference.reference_type_id, node_id,
                                          reference.node_id);
          }
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
    auto node_state = ConvertTo<scada::NodeState>(packed_node);
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
    return nullptr;

  protocol::NodeId message;
  if (!message.ParseFromString(buffer))
    return nullptr;

  const auto& type_definition_id = ConvertTo<scada::NodeId>(message);
  const auto& type_definition = node_service.GetNode(type_definition_id);
  if (!type_definition)
    return nullptr;

  for (const auto& node : {selected_node, root_node}) {
    for (auto n = node; n; n = n.parent()) {
      if (CanCreate(n, type_definition))
        return n;
    }
  }

  return nullptr;
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
