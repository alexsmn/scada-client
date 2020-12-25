#include "device_state_notifier.h"

#include "common/formula_util.h"
#include "model/devices_node_ids.h"
#include "timed_data/timed_data_service.h"

std::string ToString(DeviceState device_state) {
  static const char* kStrings[] = {"Unknown", "Disabled", "Offline", "Online"};
  static_assert(std::size(kStrings) == static_cast<size_t>(DEVICE_STATE_COUNT));
  return kStrings[static_cast<size_t>(device_state)];
}

std::wstring_view ToLocalizedString(DeviceState device_state) {
  static const std::wstring_view kStrings[] = {L"", L"Отключено", L"Нет связи",
                                               L"Есть связь"};
  static_assert(std::size(kStrings) == static_cast<size_t>(DEVICE_STATE_COUNT));
  return kStrings[static_cast<size_t>(device_state)];
}

// DeviceStateNotifier

DeviceStateNotifier::DeviceStateNotifier(TimedDataService& timed_data_service,
                                         const NodeRef& device,
                                         Callback callback)
    : callback_{std::move(callback)} {
  assert(device);
  assert(device.fetched());

  LOG_BIND_TAG(logger_, "DeviceId", ToString(device.node_id()));

  const scada::NodeId kComponentIds[] = {devices::id::DeviceType_Disabled,
                                         devices::id::DeviceType_Online};
  static_assert(_countof(kComponentIds) == FIELD_COUNT,
                "NotEnoughFieldChannelNames");

  for (size_t i = 0; i < FIELD_COUNT; ++i) {
    auto component = device[kComponentIds[i]];
    assert(component);
    if (!component)
      continue;

    TimedDataSpec& spec = specs_[i];
    spec.property_change_handler =
        [this, component, &spec](const PropertySet& properties) {
          LOG_INFO(logger_)
              << "Component data changed"
              << LOG_TAG("ComponentId", ToString(component.node_id()))
              << LOG_TAG("ComponentStatus", ToString(component.status()))
              << LOG_TAG("ComponentDisplayName", component.display_name())
              << LOG_TAG("ComponentValue", ToString(spec.current().value));
          UpdateDeviceState();
        };

    assert(!component.node_id().is_null());
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

  LOG_INFO(logger_) << "Device state changed"
                    << LOG_TAG("OldDeviceState", ToString(device_state_))
                    << LOG_TAG("NewDeviceState", ToString(device_state));

  device_state_ = device_state;
  callback_();
}
