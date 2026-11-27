#include "events/alarm_state.h"

#include "aui/translation.h"
#include "scada/event.h"

namespace events {

AlarmState AlarmStateFor(const scada::Event& event) {
  if (event.condition_id.is_null())
    return event.acked ? AlarmState::kAckedEvent : AlarmState::kUnackedEvent;
  if (event.active)
    return event.acked ? AlarmState::kAckedActive : AlarmState::kUnackedActive;
  return event.acked ? AlarmState::kReturned : AlarmState::kUnackedReturned;
}

bool IsAlarmActive(const scada::Event& event) {
  switch (AlarmStateFor(event)) {
    case AlarmState::kUnackedActive:
    case AlarmState::kAckedActive:
    case AlarmState::kUnackedEvent:
      return true;
    case AlarmState::kUnackedReturned:
    case AlarmState::kReturned:
    case AlarmState::kAckedEvent:
      return false;
  }
  return false;
}

std::u16string AlarmStateLabel(AlarmState state) {
  switch (state) {
    case AlarmState::kUnackedActive:
      return Translate("Active, unacknowledged");
    case AlarmState::kAckedActive:
      return Translate("Active, acknowledged");
    case AlarmState::kUnackedReturned:
      return Translate("Returned to normal, unacknowledged");
    case AlarmState::kReturned:
      return Translate("Returned to normal");
    case AlarmState::kUnackedEvent:
      return Translate("Event, unacknowledged");
    case AlarmState::kAckedEvent:
      return Translate("Event, acknowledged");
  }
  return {};
}

}  // namespace events
