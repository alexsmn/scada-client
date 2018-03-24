#include "device_state_notifier.h"

#include "common/formula_util.h"
#include "common/scada_node_ids.h"
#include "timed_data/timed_data_service.h"

DeviceStateNotifier::DeviceStateNotifier(TimedDataService& timed_data_service,
                                         const NodeRef& device,
                                         Callback callback)
    : callback_{std::move(callback)} {
  assert(device);
  assert(device.fetched());

  const scada::NodeId kComponentIds[] = {id::DeviceType_Disabled,
                                         id::DeviceType_Online};
  static_assert(_countof(kComponentIds) == FIELD_COUNT,
                "NotEnoughFieldChannelNames");

  for (size_t i = 0; i < FIELD_COUNT; ++i) {
    auto component = device[kComponentIds[i]];
    assert(component);
    if (!component)
      continue;

    rt::TimedDataSpec& spec = specs_[i];
    spec.property_change_handler = [this, component,
                                    &spec](const rt::PropertySet& properties) {
      LOG(INFO) << "Component " << component.id().ToString() << " = "
                << spec.current().value.get_or(std::string{});
      UpdateDeviceState();
    };

    spec.Connect(timed_data_service, component);
  }

  device_state_ = CalculateDeviceState();
}

DeviceState DeviceStateNotifier::CalculateDeviceState() const {
  const auto& disabled_tvq = specs_[FIELD_DISABLED].current();
  if (disabled_tvq.qualifier.failed())
    return DEVICE_STATE_UNKNOWN;

  bool disabled = disabled_tvq.value.get_or(false);
  if (disabled)
    return DEVICE_STATE_DISABLED;

  const auto& online_tvq = specs_[FIELD_ONLINE].current();
  if (online_tvq.qualifier.failed())
    return DEVICE_STATE_UNKNOWN;

  bool online = specs_[FIELD_ONLINE].current().value.get_or(false);
  return online ? DEVICE_STATE_ONLINE : DEVICE_STATE_OFFLINE;
}

void DeviceStateNotifier::UpdateDeviceState() {
  DeviceState device_state = CalculateDeviceState();
  if (device_state == device_state_)
    return;

  device_state_ = device_state;
  callback_();
}
