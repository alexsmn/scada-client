#pragma once

#include "model/data_items_node_ids.h"
#include "scada/node_id.h"

// True when `prop_decl_id` is one of an analog item's limits (setpoints).
//
// Limits are process data, not configuration (backlog 864): the server refuses
// to write them without the Control right, however the write arrives, so a
// surface that would otherwise offer them to a Configure-only account — the
// Свойства form, a paste — consults this to leave them alone. The server
// recognises the same four by browse name in
// scada-server-framework/node_manager/router/process_nodes.cpp.
inline bool IsLimitProperty(const scada::NodeId& prop_decl_id) {
  return prop_decl_id == scada::data_items::id::AnalogItemType_LimitLo ||
         prop_decl_id == scada::data_items::id::AnalogItemType_LimitHi ||
         prop_decl_id == scada::data_items::id::AnalogItemType_LimitLoLo ||
         prop_decl_id == scada::data_items::id::AnalogItemType_LimitHiHi;
}
