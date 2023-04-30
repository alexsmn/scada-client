#include "clipboard_util.h"

#include "base/range_util.h"
#include "base/win/clipboard.h"
#include "common/node_state.h"
#include "node_serialization.h"
#include "node_service/node_promises.h"
#include "node_service/node_util.h"
#include "remote/protocol_utils.h"
#include "services/create_tree.h"
#include "services/task_manager.h"

#include <Windows.h>
#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/adaptor/transformed.hpp>

namespace {

const UINT kNodeTreeHeaderFormat =
    ::RegisterClipboardFormat(L"CE4D311D-FB1C-4972-9EA8-3C2C1FB5091A");
const UINT kNodeTreeFormat =
    ::RegisterClipboardFormat(L"EFCAD60E-2623-4eef-8DE9-9B030DCD3AFE");

}  // namespace

std::string ReadClipboard(UINT format) {
  size_t size = 0;
  char* data = nullptr;
  if (!Clipboard().GetData(format, data, size))
    return std::string();

  std::string v(data, data + size);
  delete[] data;
  return v;
}

void CopyNodesToClipboardSync(const std::vector<NodeRef>& nodes) {
  Clipboard clipboard;

  {
    protocol::NodeId message;
    Convert(nodes.front().type_definition().node_id(), message);

    LOG(INFO) << "Set clipboard data: " << message.DebugString();
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

    LOG(INFO) << "Set clipboard data: " << message.DebugString();
    auto buffer = message.SerializePartialAsString();
    if (!clipboard.SetData(kNodeTreeFormat, buffer.data(), buffer.size()))
      LOG(ERROR) << "Can't set clipboard data";
  }
}

void CopyNodesToClipboard(const std::vector<NodeRef>& nodes) {
  assert(!nodes.empty());

  auto fetch_promises = nodes | boost::adaptors::transformed(&FetchNode);

  make_all_promise_void(std::move(fetch_promises)).then([nodes] {
    CopyNodesToClipboardSync(nodes);
  });
}

promise<> PasteNodesFromClipboardHelper(TaskManager& task_manager,
                                        scada::NodeState&& node_state) {
  auto forward_references =
      node_state.references |
      boost::adaptors::filtered(
          [](const scada::ReferenceDescription& ref) { return ref.forward; }) |
      to_vector;

  return task_manager
      .PostInsertTask({}, node_state.parent_id, node_state.type_definition_id,
                      std::move(node_state.attributes),
                      std::move(node_state.properties),
                      std::move(forward_references))
      .then([&task_manager, children = std::move(node_state.children)](
                const scada::NodeId& node_id) mutable {
        std::vector<promise<>> promises;
        for (auto& child : children) {
          assert(child.reference_type_id == scada::id::Organizes);
          child.parent_id = node_id;
          promises.emplace_back(
              PasteNodesFromClipboardHelper(task_manager, std::move(child)));
        }
        return make_all_promise_void(std::move(promises));
      });
}

promise<> PasteNodesFromClipboard(TaskManager& task_manager,
                                  const scada::NodeId& new_parent_id) {
  const auto buffer = ReadClipboard(kNodeTreeFormat);
  if (buffer.empty())
    return MakeRejectedPromise();

  protocol::NodeTree message;
  if (!message.ParseFromString(buffer))
    return MakeRejectedPromise();

  std::vector<promise<>> promises;
  for (const auto& packed_node : message.node()) {
    auto node_state = ConvertTo<scada::NodeState>(packed_node);
    assert(node_state.reference_type_id == scada::id::Organizes);
    node_state.parent_id = new_parent_id;
    promises.emplace_back(
        PasteNodesFromClipboardHelper(task_manager, std::move(node_state)));
  }

  return make_all_promise_void(std::move(promises));
}

NodeRef GetPasteParentNode(NodeService& node_service,
                           CreateTree& create_tree,
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
      if (create_tree.CanCreate(n, type_definition))
        return n;
    }
  }

  return nullptr;
}
