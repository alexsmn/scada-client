#pragma once

class TestStorage {
 public:
  explicit TestStorage(scada::NodeId root_node_id)
      : root_node_{.node_id = root_node_id} {}

  scada::NodeId Insert(scada::NodeState&& node_state) {
    if (node_state.node_id.is_null()) {
      // TODO: Set namespace based on the node type definition.
      node_state.node_id = {next_node_id_++, NamespaceIndexes::GROUP};
    }

    auto* parent_node = FindNode(node_state.parent_id);
    if (!parent_node) {
      throw std::runtime_error{"Unknown parent node ID"};
    }

    auto [_, ok] =
        node_parent_ids_.try_emplace(node_state.node_id, node_state.parent_id);
    if (!ok) {
      throw std::runtime_error{"Duplicate node ID"};
    }

    scada::NodeState& new_node_state =
        parent_node->children.emplace_back(std::move(node_state));

    return new_node_state.node_id;
  }

  scada::NodeState* FindNode(const scada::NodeId& node_id) {
    if (node_id == root_node_.node_id) {
      return &root_node_;
    }

    auto i = node_parent_ids_.find(node_id);
    if (i == node_parent_ids_.end()) {
      return nullptr;
    }

    auto* parent_node = FindNode(i->second);
    if (!parent_node) {
      return nullptr;
    }

    auto j = std::ranges::find(parent_node->children, node_id,
                               &scada::NodeState::node_id);
    return j != parent_node->children.end() ? std::to_address(j) : nullptr;
  }

  scada::NodeState root_node_;

  std::unordered_map<scada::NodeId, scada::NodeId /*parent_id*/>
      node_parent_ids_;

  scada::NumericId next_node_id_ = 1000;
};
