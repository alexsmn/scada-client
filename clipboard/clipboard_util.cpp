#include "clipboard/clipboard_util.h"

#include "base/awaitable.h"
#include "base/any_executor.h"
#include "base/thread_executor.h"
#ifdef _WIN32
#include "base/win/clipboard.h"
#endif
#include "clipboard/node_serialization.h"
#include "common/node_state.h"
#include "node_service/node_awaitable.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"
#include "remote/protocol_utils.h"
#include "services/create_tree.h"
#include "services/task_manager.h"

#ifdef _WIN32
#include <Windows.h>
#endif

#include <stdexcept>

void CopyNodesToClipboardSync(const std::vector<NodeRef>& nodes);

namespace {

#ifdef _WIN32
const UINT kNodeTreeHeaderFormat =
    ::RegisterClipboardFormat(L"CE4D311D-FB1C-4972-9EA8-3C2C1FB5091A");
const UINT kNodeTreeFormat =
    ::RegisterClipboardFormat(L"EFCAD60E-2623-4eef-8DE9-9B030DCD3AFE");
#endif

Awaitable<void> CopyNodesToClipboardAsync(std::vector<NodeRef> nodes) {
  for (const auto& node : nodes) {
    co_await FetchNode(node);
  }

  CopyNodesToClipboardSync(nodes);
}

Awaitable<void> PasteNodesFromNodeStateRecursiveAsync(
    TaskManager& task_manager,
    scada::NodeState node_state);

Awaitable<void> PasteChildrenAsync(TaskManager& task_manager,
                                   std::vector<scada::NodeState> children,
                                   const scada::NodeId& parent_id) {
  for (auto& child : children) {
    assert(child.reference_type_id == scada::id::Organizes);
    child.parent_id = parent_id;
    co_await PasteNodesFromNodeStateRecursiveAsync(task_manager,
                                                   std::move(child));
  }
}

Awaitable<void> PasteNodesFromNodeStateRecursiveAsync(
    TaskManager& task_manager,
    scada::NodeState node_state) {
  auto executor = co_await boost::asio::this_coro::executor;

  std::erase_if(
      node_state.references,
      [](const scada::ReferenceDescription& ref) { return !ref.forward; });

  auto children = std::exchange(node_state.children, {});

  // `PostInsertTask` must take a node state with no children.
  auto node_id = co_await task_manager.PostInsertTask(node_state);
  if (!node_id.ok()) {
    co_return;
  }
  co_await PasteChildrenAsync(task_manager, std::move(children), *node_id);
}

Awaitable<void> PasteNodesFromNodeTreeAsync(
    TaskManager& task_manager,
    const scada::NodeId& new_parent_id,
    const protocol::NodeTree& node_tree) {
  for (const auto& packed_node : node_tree.node()) {
    auto node_state = ConvertTo<scada::NodeState>(packed_node);
    assert(node_state.reference_type_id == scada::id::Organizes);
    node_state.parent_id = new_parent_id;
    co_await PasteNodesFromNodeStateRecursiveAsync(task_manager,
                                                   std::move(node_state));
  }
}

Awaitable<void> PasteNodesFromClipboardAsync(TaskManager& task_manager,
                                             scada::NodeId new_parent_id,
                                             std::string buffer) {
  if (buffer.empty())
    throw std::runtime_error{"Clipboard does not contain node data"};

  protocol::NodeTree node_tree;
  if (!node_tree.ParseFromString(buffer))
    throw std::runtime_error{"Clipboard node data is invalid"};

  co_await PasteNodesFromNodeTreeAsync(task_manager, new_parent_id, node_tree);
}

}  // namespace

std::string ReadClipboard(
#ifdef _WIN32
    UINT format
#else
    unsigned
#endif
) {
#ifdef _WIN32
  size_t size = 0;
  char* data = nullptr;
  if (!Clipboard().GetData(format, data, size))
    return std::string();

  std::string v(data, data + size);
  delete[] data;
  return v;
#else
  return {};
#endif
}

protocol::NodeTree BuildNodeTree(const std::vector<NodeRef>& nodes) {
  std::vector<scada::NodeState> browse_nodes;
  browse_nodes.reserve(nodes.size());

  for (auto& node : nodes) {
    browse_nodes.emplace_back();
    NodeToData(node, browse_nodes.back(), true, true);
  }

  protocol::NodeTree message;
  Convert(browse_nodes, *message.mutable_node());
  return message;
}

void CopyNodesToClipboardSync(const std::vector<NodeRef>& nodes) {
#ifdef _WIN32
  Clipboard clipboard;

  {
    protocol::NodeId message;
    Convert(nodes.front().type_definition().node_id(), message);

    auto buffer = message.SerializePartialAsString();
    clipboard.SetData(kNodeTreeHeaderFormat, buffer.data(), buffer.size());
  }

  {
    auto node_tree = BuildNodeTree(nodes);

    auto buffer = node_tree.SerializePartialAsString();
    clipboard.SetData(kNodeTreeFormat, buffer.data(), buffer.size());
  }
#else
  (void)nodes;
#endif
}

void CopyNodesToClipboard(const std::vector<NodeRef>& nodes) {
  assert(!nodes.empty());

  CoSpawn(ThreadExecutor{}, [nodes]() -> Awaitable<void> {
    try {
      co_await CopyNodesToClipboardAsync(nodes);
    } catch (...) {
    }
  });
}

Awaitable<void> PasteNodesFromNodeStateRecursive(TaskManager& task_manager,
                                                 scada::NodeState&& node_state) {
  co_await PasteNodesFromNodeStateRecursiveAsync(task_manager,
                                                std::move(node_state));
}

Awaitable<void> PasteNodesFromNodeTree(TaskManager& task_manager,
                                       const scada::NodeId& new_parent_id,
                                       const protocol::NodeTree& node_tree) {
  co_await PasteNodesFromNodeTreeAsync(task_manager, new_parent_id, node_tree);
}

Awaitable<void> PasteNodesFromClipboard(TaskManager& task_manager,
                                        const scada::NodeId& new_parent_id) {
#ifdef _WIN32
  co_await PasteNodesFromClipboardAsync(task_manager, new_parent_id,
                                       ReadClipboard(kNodeTreeFormat));
#else
  (void)task_manager;
  (void)new_parent_id;
  throw std::runtime_error{"Clipboard is not supported on this platform"};
#endif
}

NodeRef GetPasteParentNode(NodeService& node_service,
                           CreateTree& create_tree,
                           const NodeRef& selected_node,
                           const NodeRef& root_node) {
#ifdef _WIN32
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
      if (create_tree.CanCreate(n, type_definition))
        return n;
    }
  }

  return nullptr;
#else
  (void)node_service;
  (void)create_tree;
  (void)selected_node;
  (void)root_node;
  return nullptr;
#endif
}
