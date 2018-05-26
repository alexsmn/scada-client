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
                bool recursive);

scada::NodeState FromProto(const protocol::Node& source);
void ToProto(const scada::NodeState& source, protocol::Node& target);
