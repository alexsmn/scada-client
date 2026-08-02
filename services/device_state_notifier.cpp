#include "device_state_notifier.h"

#include "base/check.h"
#include "base/debug_util.h"
#include "model/node_id_util.h"
#include "node_service/node_ref.h"
#include "timed_data/timed_data_service.h"

std::string ToString(DeviceState device_state) {
  static const char* kStrings[] = {"Unknown", "Disabled", "Offline", "Online"};
  static_assert(std::size(kStrings) == static_cast<size_t>(DeviceState::Count));
  return kStrings[static_cast<size_t>(device_state)];
}

std::u16string_view ToLocalizedString(DeviceState device_state) {
  static const std::u16string_view kStrings[] = {
      u"", u"\u041e\u0442\u043a\u043b\u044e\u0447\u0435\u043d\u043e",
      u"\u041d\u0435\u0442 \u0441\u0432\u044f\u0437\u0438",
      u"\u0415\u0441\u0442\u044c \u0441\u0432\u044f\u0437\u044c"};
  static_assert(std::size(kStrings) == static_cast<size_t>(DeviceState::Count));
  return kStrings[static_cast<size_t>(device_state)];
}

// DeviceStateNotifier

DeviceStateNotifier::DeviceStateNotifier(TimedDataService& timed_data_service,
                                         const NodeRef& device,
                                         Callback callback)
    : callback_{std::move(callback)} {
  scada::base::Check(device);
  scada::base::Check(device.fetched());

  LOG_BIND_TAG(logger_, "DeviceId", ToString(device.node_id()));

  // The runtime status components are the device's aggregate variables Disabled
  // and Online (declared on DeviceType in the model nodeset). Their instance
  // node ids are nested ids of the device keyed by browse name — the server
  // routes reads/monitors of `MakeNestedNodeId(device, <browse-name>)` to the
  // device's service variable (aggregate_node_manager builds the id the same
  // way, and device_io_manager resolves it back via FindAggregateDecl).
  //
  // Address them by constructed node id rather than by browsing
  // `device[declaration]`: a remote node service fetches lazily, so neither the
  // instance children nor the DeviceType aggregate declarations are fetched
  // right after the device node loads (only NodeOnly), and the synchronous
  // aggregate lookup returns null. (The static reference-type subtype predicate
  // makes the aggregate *filter* resolve, but the child nodes themselves are
  // still absent until a ChildrenOnly fetch, which the notifier does not do.)
  // The browse-resolved node id would be identical to this one.
  const scada::NodeId& device_id = device.node_id();
  const std::string_view kComponentBrowseNames[] = {"Disabled", "Online"};
  static_assert(std::size(kComponentBrowseNames) == FIELD_COUNT,
                "NotEnoughFieldChannelNames");

  for (size_t i = 0; i < FIELD_COUNT; ++i) {
    scada::NodeId component_id =
        MakeNestedNodeId(device_id, kComponentBrowseNames[i]);

    TimedDataSpec& spec = specs_[i];
    spec.property_change_handler =
        [this, component_id, &spec](const PropertySet& properties) {
          LOG_INFO(logger_)
              << "Component data changed"
              << LOG_TAG("ComponentId", ToString(component_id))
              << LOG_TAG("ComponentValue", ToString(spec.current().value));
          UpdateDeviceState(true);
        };

    spec.Connect(timed_data_service, component_id);
  }

  UpdateDeviceState(false);
}

DeviceState DeviceStateNotifier::CalculateDeviceState() const {
  // Only a positively read Disabled=true reports Disabled. When Disabled cannot
  // be read at all, fall through to Online rather than reporting Unknown: an
  // unreadable Disabled must not mask a perfectly good Online reading. A server
  // may refuse the Disabled monitor while serving Online — the two are separate
  // variables — and reporting Unknown then hides a device that is plainly up.
  const auto& disabled_tvq = specs_[FIELD_DISABLED].current();
  if (!disabled_tvq.qualifier.failed() && disabled_tvq.value.get_or(false))
    return DeviceState::Disabled;

  const auto& online_tvq = specs_[FIELD_ONLINE].current();
  if (online_tvq.qualifier.failed())
    return DeviceState::Unknown;

  // No Online value delivered yet (the component is absent, or the monitored
  // value has not arrived): the state is genuinely unknown, not Offline. A
  // premature Offline both mislabels a just-loaded device and masks the
  // hardware tree's Value-attribute snapshot fallback (DeviceStateFromNode).
  if (online_tvq.value.is_null())
    return DeviceState::Unknown;

  bool online = online_tvq.value.get_or(false);
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
