#pragma once

#include "scada/node_id.h"

#include <optional>

class NodeRef;

// The "Two-stage control" (`DataItemType_OutputTwoStaged`) value a data item
// should be created with, derived from the device it is bound to and its type.
//
// The nodeset default is Yes, and the Server stores that default on every node
// created without the property. But since 2.6 the Server refuses the select
// stage wherever the device cannot select, so a Yes on such an object makes it
// uncontrollable until an engineer turns it off. This answers No where the
// creating dialog already knows the select would be refused:
//
// - any data item on a MODBUS device — MODBUS has no select-before-operate,
//   and the MODBUS tier refuses every select;
// - an analog data item on an IEC 60870-5 device — its command is a set-point
//   (`P_ME_*`), and only the single command `C_SC_NA_1` can be selected.
//
// It answers `std::nullopt` — keep the default — everywhere else, including
// every IEC 61850 device: whether an IEC 61850 object selects is its control
// model (`ctlModel`), which the Server reads from the IED at run time and the
// configuration does not hold. A null or unresolved `device` also answers
// `std::nullopt`.
//
// It decides only the value a new object starts with. Existing objects keep
// whatever they store; the property stays editable.
std::optional<bool> GetOutputTwoStagedDefault(
    const NodeRef& device,
    const scada::NodeId& item_type_definition_id);
