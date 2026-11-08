#pragma once

#include "scada/node_id.h"

#include <optional>
#include <string_view>

class NodeRef;
class NodeService;

// The "Two-stage control" (`DataItemType_OutputTwoStaged`) value a data item
// should be created with, derived from the device it is bound to and its type.
//
// The nodeset default is Yes, and the Server stores that default on every node
// created without the property. But since 2.6 the Server refuses the select
// stage wherever the device cannot select, so a Yes on such an object makes it
// uncontrollable until an engineer turns it off. This answers No where the
// Client already knows the select would be refused:
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
// It decides only the value a new or re-bound object starts with; see
// `GetOutputTwoStagedOnRebind` for the second. The property stays editable.
//
// `TaskManagerImpl` applies it to every insert and every channel edit, so no
// create or edit path has to remember to (backlog 1016).
std::optional<bool> GetOutputTwoStagedDefault(
    const NodeRef& device,
    const scada::NodeId& item_type_definition_id);

// The device a data item's command is routed to, read from its channel
// formulas: the device of the Output channel, or of the Input1 channel when
// the item has no Output yet (the bulk and multi create bind only Input1, and
// the engineer then sets Output on the same device). Null when neither formula
// names a single channel of a device.
//
// A MODBUS or IEC 60870-5 channel is a nested id under its device and resolves
// without a fetch. Any other channel (an IEC 61850 data object is a real node)
// resolves through the parents `node_service` has already fetched, and is null
// when they are not.
scada::NodeId GetDataItemDeviceId(NodeService& node_service,
                                  std::string_view output_formula,
                                  std::string_view input_formula);

// Whether an existing data item's "Two-stage control" should change when its
// device changes, and to what. `stored` is the value the item holds;
// `old_default` and `new_default` are `GetOutputTwoStagedDefault` for the old
// and the new device (`std::nullopt` meaning the nodeset default, Yes).
//
// The property is treated as still at its default when `stored` equals the
// value the Client would have created the item with for the OLD device. Only
// then does it follow the new device; any other value is the engineer's
// choice and is kept. Answers `std::nullopt` when nothing should be written.
//
// The configuration records no "was this edited" bit, so the rule cannot tell
// an engineer's explicit Yes on an unbound item from the default Yes. It errs
// toward controllability: such an item bound to a MODBUS device becomes No,
// which is the only value that device accepts.
std::optional<bool> GetOutputTwoStagedOnRebind(bool stored,
                                               std::optional<bool> old_default,
                                               std::optional<bool> new_default);
