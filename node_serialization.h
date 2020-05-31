#pragma once

#include "common/node_ref.h"

namespace protocol {
class Node;
}  // namespace protocol

namespace scada {
struct NodeState;
}  // namespace scada

void NodeToData(const NodeRef& source,
                scada::NodeState& target,
                bool recursive,
                bool ignore_browse_name);

void Convert(const protocol::Node& source, scada::NodeState& target);
void Convert(const scada::NodeState& source, protocol::Node& target);
