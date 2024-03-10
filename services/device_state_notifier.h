#pragma once

#include "base/boost_log.h"
#include "timed_data/timed_data_spec.h"

#include <memory>

class TimedDataService;

enum class DeviceState { Unknown, Disabled, Offline, Online, Count };

// Notifies observers about changes of ONLINE and ENABLED device fields.
class DeviceStateNotifier {
 public:
  using Callback = std::function<void()>;

  DeviceStateNotifier(TimedDataService& timed_data_service,
                      const NodeRef& device,
                      Callback callback);

  DeviceStateNotifier(const DeviceStateNotifier&) = delete;
  DeviceStateNotifier& operator=(const DeviceStateNotifier&) = delete;

  DeviceState device_state() const { return device_state_; }

 private:
  enum { FIELD_DISABLED, FIELD_ONLINE, FIELD_COUNT };

  DeviceState CalculateDeviceState() const;
  void UpdateDeviceState(bool notify);

  BoostLogger logger_{LOG_NAME("DeviceStateNotifier")};

  const Callback callback_;

  TimedDataSpec specs_[FIELD_COUNT];

  DeviceState device_state_ = DeviceState::Unknown;
};

std::string ToString(DeviceState device_state);

std::u16string_view ToLocalizedString(DeviceState device_state);
