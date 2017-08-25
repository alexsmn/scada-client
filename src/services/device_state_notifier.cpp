#include "client/services/device_state_notifier.h"

#include "common/formula_util.h"
#include "common/timed_data/timed_data_service.h"
#include "common/scada_node_ids.h"

DeviceStateNotifier::DeviceStateNotifier(TimedDataService& timed_data_service, const NodeRef& device, Callback callback)
    : callback_{std::move(callback)} {
  const scada::NodeId kComponentIds[] = {id::DeviceType_Enabled, id::DeviceType_Online};
  static_assert(_countof(kComponentIds) == FIELD_COUNT, "NotEnoughFieldChannelNames");

  assert(device.fetched());

  for (size_t i = 0; i < FIELD_COUNT; ++i) {
    auto& component = device[kComponentIds[i]];
    if (!component)
      continue;

    rt::TimedDataSpec& spec = specs_[i];
    spec.set_delegate(this);
    spec.param = reinterpret_cast<void*>(i);

    std::string formula = MakeNodeIdFormula(component.id());
    try {
      spec.Connect(timed_data_service, formula);
    } catch (const std::exception&) {
    }
  }

  device_state_ = CalculateDeviceState();
}

DeviceState DeviceStateNotifier::CalculateDeviceState() const {
  const auto& enabled_tvq = specs_[FIELD_ENABLED].current();
  if (enabled_tvq.qualifier.failed() || enabled_tvq.value.is_null())
    return DEVICE_STATE_UNKNOWN;

  bool enabled = enabled_tvq.value.get_or(false);
  if (!enabled)
    return DEVICE_STATE_DISABLED;

  const auto& online_tvq = specs_[FIELD_ONLINE].current();

  if (online_tvq.qualifier.failed() || online_tvq.value.is_null())
    return DEVICE_STATE_UNKNOWN;

  bool online = specs_[FIELD_ONLINE].current().value.get_or(false);
  return online ? DEVICE_STATE_ONLINE : 
                  DEVICE_STATE_OFFLINE;
}

void DeviceStateNotifier::OnPropertyChanged(
                                       rt::TimedDataSpec& spec,
                                       const rt::PropertySet& properties) {
  LOG(INFO) << "Spec " << reinterpret_cast<unsigned>(spec.param)
            << " changed value to " << spec.current().value.get_or(std::string{});

  DeviceState device_state = CalculateDeviceState();
  if (device_state == device_state_)
    return;

  device_state_ = device_state;
  callback_();
}
