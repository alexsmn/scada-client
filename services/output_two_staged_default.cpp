#include "services/output_two_staged_default.h"

#include "common/formula_util.h"
#include "model/data_items_node_ids.h"
#include "model/devices_node_ids.h"
#include "model/node_id_util.h"
#include "node_service/node_ref.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"

namespace {

// Bounds the parent walk for a non-nested channel. An IEC 61850 data object
// sits a few levels under its device (device → logical device → logical node
// → data object); anything deeper is not a device channel.
constexpr int kMaxChannelDepth = 8;

scada::NodeId GetChannelDeviceId(NodeService& node_service,
                                 std::string_view formula) {
  if (formula.empty())
    return {};

  const scada::NodeId channel_id = GetFormulaSingleNodeId(formula);
  if (channel_id.is_null())
    return {};

  scada::NodeId root_id;
  std::string_view nested_name;
  if (scada::GetRootNestedNodeId(channel_id, root_id, nested_name))
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

std::optional<bool> GetOutputTwoStagedDefault(
    const NodeRef& device,
    const scada::NodeId& item_type_definition_id) {
  if (!device)
    return std::nullopt;

  if (IsInstanceOf(device, scada::devices::id::ModbusDeviceType))
    return false;

  // An analog item's IEC 60870-5 command carries a number, so the tier sends
  // it as a set-point, which `Iec60870DeviceStateClient` refuses to select. A
  // discrete item's command is `C_SC_NA_1`, which does select, so it keeps the
  // default.
  if (IsInstanceOf(device, scada::devices::id::Iec60870DeviceType) &&
      item_type_definition_id == scada::data_items::id::AnalogItemType) {
    return false;
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
  // `std::nullopt` is the nodeset default, which is Yes.
  if (stored != old_default.value_or(true))
    return std::nullopt;

  const bool value = new_default.value_or(true);
  if (value == stored)
    return std::nullopt;
  return value;
}
