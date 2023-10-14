#pragma once

#include "scada/node_id.h"

namespace vidicon {

struct DataPointAddress;

scada::NodeId ToNodeId(const DataPointAddress& address);

}  // namespace vidicon