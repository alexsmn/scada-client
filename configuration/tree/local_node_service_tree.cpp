#include "configuration/tree/local_node_service_tree.h"

#include "node_service/node_service.h"
#include "scada/standard_node_ids.h"

#include <boost/json.hpp>

#include <string>
#include <utility>

void LocalNodeServiceTree::SharedData::LoadFromJson(
    const boost::json::value& root) {
  for (const auto& [key, val] : root.at("tree").as_object()) {
    auto key_str = std::string(key);
    scada::NumericId id = 0;
    scada::NamespaceIndex ns = 1;
    if (auto dot = key_str.find('.'); dot != std::string::npos) {
      ns = static_cast<scada::NamespaceIndex>(
          std::stoi(key_str.substr(0, dot)));
      id = static_cast<scada::NumericId>(std::stoi(key_str.substr(dot + 1)));
    } else {
      id = static_cast<scada::NumericId>(std::stoi(key_str));
    }

    std::vector<scada::NodeId> child_ids;
    for (const auto& child : val.as_array()) {
      child_ids.push_back(
          scada::NodeId{static_cast<scada::NumericId>(child.as_int64()), 1});
    }
    children[scada::NodeId{id, ns}] = std::move(child_ids);
  }
}

LocalNodeServiceTree::LocalNodeServiceTree(
    NodeService& node_service,
    NodeRef root_node,
    std::shared_ptr<const SharedData> data)
    : node_service_{node_service},
      root_node_{std::move(root_node)},
      data_{std::move(data)} {}

LocalNodeServiceTree::~LocalNodeServiceTree() = default;

NodeServiceTreeFactory LocalNodeServiceTree::MakeFactory(
    NodeService& node_service,
    std::shared_ptr<const SharedData> data) {
  return [&node_service, data = std::move(data)](
             NodeServiceTreeImplContext&& context)
             -> std::unique_ptr<NodeServiceTree> {
    return std::make_unique<LocalNodeServiceTree>(
        node_service, context.root_node_, data);
  };
}

NodeRef LocalNodeServiceTree::GetRoot() const {
  return root_node_;
}

bool LocalNodeServiceTree::HasChildren(const NodeRef& node) const {
  auto it = data_->children.find(node.node_id());
  return it != data_->children.end() && !it->second.empty();
}

std::vector<NodeServiceTree::ChildRef> LocalNodeServiceTree::GetChildren(
    const NodeRef& node) const {
  std::vector<ChildRef> result;
  auto it = data_->children.find(node.node_id());
  if (it == data_->children.end())
    return result;

  const scada::NodeId organizes{scada::id::Organizes, 0};
  for (const auto& child_id : it->second) {
    NodeRef child_ref = node_service_.GetNode(child_id);
    if (!child_ref)
      continue;
    result.push_back({organizes, /*forward=*/true, child_ref});
  }
  return result;
}
