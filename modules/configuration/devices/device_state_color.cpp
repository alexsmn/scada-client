#include "configuration/devices/device_state_color.h"

#include "model/devices_node_ids.h"
#include "node_service/node_ref.h"
#include "scada/standard_node_ids.h"
#include "scada/variant.h"

#include <string_view>

namespace {

// True when a device flag reads as set. A real server sends a Boolean Variant;
// the headless fixture stores the flag as a number, so accept a non-zero
// integer/double too.
bool VariantIsTruthy(const scada::Variant& value) {
  if (bool flag; value.get(flag))
    return flag;
  if (scada::Int32 i; value.get(i))
    return i != 0;
  if (double d; value.get(d))
    return d != 0.0;
  return false;
}

// Resolves a device's runtime component. The live app models these as aggregate
// (HasComponent) variables resolved by the type declaration; the headless
// fixture attaches them as plain children, matched by browse name.
NodeRef ResolveComponent(const NodeRef& device,
                         const scada::NodeId& declaration_id,
                         std::string_view browse_name) {
  if (NodeRef component = device[declaration_id])
    return component;
  for (const NodeRef& child : device.targets()) {
    if (child.browse_name().name() == browse_name)
      return child;
  }
  return {};
}

}  // namespace

std::optional<scada::aui::Quality> DeviceStateQuality(DeviceState state) {
  switch (state) {
    case DeviceState::Online:
      return scada::aui::Quality::kGood;
    case DeviceState::Offline:
      return scada::aui::Quality::kBad;
    case DeviceState::Disabled:
      return scada::aui::Quality::kUncertain;
    case DeviceState::Unknown:
    case DeviceState::Count:
      break;
  }
  return std::nullopt;
}

DeviceState DeviceStateFromNode(const NodeRef& device) {
  if (!device)
    return DeviceState::Unknown;

  NodeRef disabled = ResolveComponent(
      device, scada::devices::id::DeviceType_Disabled, "Disabled");
  if (disabled && VariantIsTruthy(disabled.value()))
    return DeviceState::Disabled;

  NodeRef online =
      ResolveComponent(device, scada::devices::id::DeviceType_Online, "Online");
  if (!online)
    return DeviceState::Unknown;

  return VariantIsTruthy(online.value()) ? DeviceState::Online
                                         : DeviceState::Offline;
}
