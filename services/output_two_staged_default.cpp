#include "services/output_two_staged_default.h"

#include "model/data_items_node_ids.h"
#include "model/devices_node_ids.h"
#include "node_service/node_ref.h"
#include "node_service/node_util.h"

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
