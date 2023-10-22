#pragma once

#include "scada/node_id.h"

#include <set>

// TODO: Move away.
typedef std::set<scada::NodeId> NodeIdSet;

inline NodeIdSet MakeNodeIdSet(const scada::NodeId& node_id) {
  NodeIdSet result;
  result.insert(node_id);
  return result;
}
