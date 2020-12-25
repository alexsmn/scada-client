#pragma once

#include <memory>

#include "address_space/node_observer.h"
#include "base/boost_log.h"
#include "core/configuration_types.h"
#include "timed_data/timed_data_spec.h"

class TimedDataService;

enum DeviceState {
  DEVICE_STATE_UNKNOWN,
  DEVICE_STATE_DISABLED,
  DEVICE_STATE_OFFLINE,
  DEVICE_STATE_ONLINE,
  DEVICE_STATE_COUNT,
};

// Notifies observers about changes of ONLINE and ENABLED device fields.
class DeviceStateNotifier {
 public:
  using Callback = std::function<void()>;

  DeviceStateNotifier(TimedDataService& timed_data_service,
                      const NodeRef& device,
                      Callback callback);

  DeviceState device_state() const { return device_state_; }

 private:
  enum { FIELD_DISABLED, FIELD_ONLINE, FIELD_COUNT };

  DeviceState CalculateDeviceState() const;
  void UpdateDeviceState();

  BoostLogger logger_{LOG_NAME("DeviceStateNotifier")};

  const Callback callback_;

  TimedDataSpec specs_[FIELD_COUNT];

  DeviceState device_state_ = DEVICE_STATE_UNKNOWN;

  DISALLOW_COPY_AND_ASSIGN(DeviceStateNotifier);
};

std::string ToString(DeviceState device_state);

std::wstring_view ToLocalizedString(DeviceState device_state);
