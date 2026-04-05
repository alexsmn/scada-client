#pragma once

#include "common/node_state.h"
#include "scada/node_attributes.h"

struct DiffData {
  struct Reference {
    scada::NodeId reference_type_id;
    scada::NodeId delete_target_id;
    scada::NodeId add_target_id;
  };

  struct Node {
    scada::NodeId id;
    scada::NodeId type_id;
    scada::NodeId parent_id;
    scada::NodeAttributes attrs;
    scada::NodeProperties props;
    std::vector<Reference> refs;
  };

  bool IsEmpty() const {
    return create_nodes.empty() && modify_nodes.empty() && delete_nodes.empty();
  }

  std::vector<scada::NodeState> create_nodes;
  std::vector<Node> modify_nodes;
  std::vector<scada::NodeId> delete_nodes;
};
