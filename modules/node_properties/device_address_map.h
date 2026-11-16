#pragma once

#include "base/any_executor.h"
#include "base/awaitable.h"
#include "parameter_form/address_map_row.h"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace scada {
class NodeId;
}
class NodeRef;

// The signal-type tag (TS for a discrete item, TI for an analog item) for a
// data-item type definition; empty for types without a simple tag. Pure, so it
// is unit-testable without a node service.
std::u16string SignalTypeTag(const scada::NodeId& type_definition_id);

// The protocols whose polled channels are addresses nested under the device's
// own node id, and so can be read off a channel formula without a fetch.
enum class PolledChannelProtocol {
  kIec60870,
  kModbus,
};

// The address a data item's channel formula polls on `device_id`, as the
// address map shows it. The channel is a node id nested under the device's
// (`<device>!<address>`), and the address part is read as the tier reads it:
// for IEC 60870-5 the information-object address (`105` and `FLOAT:105` are
// both 105); for MODBUS the register or coil without its data type and
// options (`FLOAT:40001;swapwords` is 40001, `HOLDREG:1:3+2` stays as it is).
// Empty when the formula is not a single channel, the channel is on another
// device, or its address does not parse. Pure, so it is unit-testable without
// a node service.
std::optional<std::u16string> GetPolledChannelAddress(
    PolledChannelProtocol protocol,
    const scada::NodeId& device_id,
    std::string_view formula);

// Builds `device`'s address-map rows — the data item's name and type tag, the
// address (IOA, MODBUS register), and the item's node id — for the device
// parameter form's address-map tab and Device watch's frame decoding.
//
// Two kinds of row, told apart by AddressMapRow::kind: the device's
// transmission items first, then, for an IEC 60870-5 or MODBUS device, every
// data item whose channel (Input1, or its backup Input2) is on the device,
// found by walking the DataItems folder. Fetches what it reads, so it is safe
// to call on a freshly-opened device. Returns an empty list for a device with
// neither.
Awaitable<std::vector<AddressMapRow>> BuildDeviceAddressMap(
    AnyExecutor executor,
    NodeRef device);
