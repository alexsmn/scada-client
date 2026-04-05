#include "device_state_notifier.h"

#include "common/formula_util.h"
#include "model/devices_node_ids.h"
#include "timed_data/timed_data_service.h"

#include "base/debug_util.h"

std::string ToString(DeviceState device_state) {
  static const char* kStrings[] = {"Unknown", "Disabled", "Offline", "Online"};
  static_assert(std::size(kStrings) == static_cast<size_t>(DeviceState::Count));
  return kStrings[static_cast<size_t>(device_state)];
}

std::u16string_view ToLocalizedString(DeviceState device_state) {
  static const std::u16string_view kStrings[] = {u"", u"Отключено",
                                                 u"Нет связи", u"Есть связь"};
  static_assert(std::size(kStrings) == static_cast<size_t>(DeviceState::Count));
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
          UpdateDeviceState(true);
        };

    assert(!component.node_id().is_null());
    spec.Connect(timed_data_service, component);
  }

  UpdateDeviceState(false);
}

DeviceState DeviceStateNotifier::CalculateDeviceState() const {
  const auto& disabled_tvq = specs_[FIELD_DISABLED].current();
  if (disabled_tvq.qualifier.failed())
    return DeviceState::Unknown;

  bool disabled = disabled_tvq.value.get_or(false);
  if (disabled)
    return DeviceState::Disabled;

  const auto& online_tvq = specs_[FIELD_ONLINE].current();
  if (online_tvq.qualifier.failed())
    return DeviceState::Unknown;

  bool online = specs_[FIELD_ONLINE].current().value.get_or(false);
  return online ? DeviceState::Online : DeviceState::Offline;
}

void DeviceStateNotifier::UpdateDeviceState(bool notify) {
  DeviceState device_state = CalculateDeviceState();
  if (device_state == device_state_)
    return;

  LOG_INFO(logger_) << "Device state changed"
                    << LOG_TAG("OldDeviceState", ToString(device_state_))
                    << LOG_TAG("NewDeviceState", ToString(device_state));

  device_state_ = device_state;

  if (notify)
    callback_();
}
