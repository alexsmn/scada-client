#include "services/output_two_staged_default.h"

#include "common/formula_util.h"
#include "model/data_items_node_ids.h"
#include "model/devices_node_ids.h"
#include "model/namespaces.h"
#include "model/node_id_util.h"
#include "node_service/node_ref.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"

namespace {

// Bounds the parent walk for a non-nested channel. An IEC 61850 data object
// sits a few levels under its device (device → logical device → logical node
// → data object); anything deeper is not a device channel.
constexpr int kMaxChannelDepth = 8;

// The OPC server an OPC item address belongs to. A Classic OPC channel is not
// a node under a device: it is a string id in the OPC namespace, spelled
// `[\\machine\]ProgID[\ItemID]` (`opc_client::Address`; parsed for real by
// `scada::opc::ParseOpcNodeId`, which is Windows-only and so not linked here).
// The server is that address up to the item, so two items of one server
// resolve to the same "device". An address with no item is the server itself.
scada::NodeId GetOpcServerId(const scada::NodeId& item_id) {
  const scada::String& address = item_id.string_id();
  std::size_t prog_id_start = 0;
  if (address.starts_with("\\\\")) {
    const std::size_t machine_end = address.find('\\', 2);
    if (machine_end == scada::String::npos)
      return item_id;
    prog_id_start = machine_end + 1;
  }
  const std::size_t prog_id_end = address.find('\\', prog_id_start);
  if (prog_id_end == scada::String::npos)
    return item_id;
  return scada::NodeId{address.substr(0, prog_id_end),
                       scada::NamespaceIndexes::OPC};
}

scada::NodeId GetChannelDeviceId(NodeService& node_service,
                                 std::string_view formula) {
  if (formula.empty())
    return {};

  const scada::NodeId channel_id = GetFormulaSingleNodeId(formula);
  if (channel_id.is_null())
    return {};

  scada::NodeId root_id;
  std::string_view nested_name;
  if (!scada::GetRootNestedNodeId(channel_id, root_id, nested_name))
    root_id = channel_id;

  if (IsOpcDeviceId(root_id))
    return GetOpcServerId(root_id);

  if (root_id != channel_id)
    return root_id;

  NodeRef node = node_service.GetNode(channel_id);
  for (int i = 0; node && i < kMaxChannelDepth; ++i) {
    if (IsInstanceOf(node, scada::devices::id::DeviceType))
      return node.node_id();
    node = node.parent();
  }
  return {};
}

}  // namespace

bool IsOpcDeviceId(const scada::NodeId& device_id) {
  return device_id.namespace_index() == scada::NamespaceIndexes::OPC &&
         device_id.is_string();
}

std::optional<bool> GetOutputTwoStagedDefault(
    const NodeRef& device,
    const scada::NodeId& item_type_definition_id) {
  if (!device)
    return std::nullopt;

  // Classic OPC DA has no select-before-operate, and the OPC tier refuses
  // every select (`OpcIoManager::Write`, backlog 994). Decided from the id
  // alone: an OPC server is not a typed device node.
  if (IsOpcDeviceId(device.node_id()))
    return false;

  if (IsInstanceOf(device, scada::devices::id::ModbusDeviceType))
    return false;

  // An analog item's IEC 60870-5 command carries a number, so the tier sends
  // it as a set-point, which `Iec60870DeviceStateClient` refuses to select. A
  // discrete item's command is `C_SC_NA_1`, which does select: that is a known
  // Yes, not an unknown, which is what lets a re-bind tell it from IEC 61850.
  if (IsInstanceOf(device, scada::devices::id::Iec60870DeviceType)) {
    return item_type_definition_id != scada::data_items::id::AnalogItemType;
  }

  return std::nullopt;
}

scada::NodeId GetDataItemDeviceId(NodeService& node_service,
                                  std::string_view output_formula,
                                  std::string_view input_formula) {
  return GetChannelDeviceId(
      node_service, output_formula.empty() ? input_formula : output_formula);
}

std::optional<bool> GetOutputTwoStagedOnRebind(
    bool stored,
    std::optional<bool> old_default,
    std::optional<bool> new_default) {
  // An item on an unknown or no device was created with the nodeset default,
  // Yes, so that is what "still the default" means for it.
  if (stored != old_default.value_or(true))
    return std::nullopt;

  // The new device's answer is unknown (IEC 61850: its control model is on
  // the IED). Turning a one-stage item back to Yes there would make every
  // direct-control object uncontrollable, and an engineer who knows the IED
  // selects can turn it on; so the stored value is kept (backlog 1028).
  if (!new_default.has_value())
    return std::nullopt;

  if (*new_default == stored)
    return std::nullopt;
  return new_default;
}
