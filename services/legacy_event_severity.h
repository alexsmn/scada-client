#pragma once

#include "base/awaitable.h"
#include "scada/data_services.h"
#include "scada/event.h"

#include <memory>
#include <optional>
#include <span>

namespace scada {
class AttributeService;
}

// Maps an event severity a Server from before 2.6 sent onto the scale the
// Client bands on.
//
// 2.6 moved event severity to the OPC UA BaseEventType scale, 1-1000 with the
// Client's Warning at 600 and Critical at 800 (0ba5c1b03,
// `core/scada/event.h`). A 2.5.7 Server still sends its own 0-100 scale --
// Warning 60, Critical 80, with 0 for "silent" -- so the factor is exactly ten.
// 0 is not a valid OPC UA severity and becomes 1, the lowest; nothing a Server
// sends can exceed 1000. The same formula the 2.6 Server used to migrate stored
// severities once
// (`s' = clamp(10*s, 1, 1000)`), so an event reads the same whichever Server
// version stored it.
scada::EventSeverity RescaleLegacySeverity(unsigned severity);

// Rescales every event in `events` in place with `RescaleLegacySeverity`.
void RescaleLegacySeverities(std::span<scada::Event> events);

// Remembers, per signed-in session, whether the Server is from before 2.6.
//
// There is no version number on the wire to go on: 2.6 and 2.5.7 share
// connection protocol 20.0 (see services/server_compatibility.h). So the
// Server is identified the way the summary's Time Average check identifies it:
// by reading a node 2.6 added, the per-item Control object's type, which a
// 2.5.7 Server refuses as an unknown node. Asked once, on the first event the
// Client receives, and remembered; a probe that fails for any other reason
// (the session dropped mid-read) decides nothing and is asked again next time.
class LegacySeverityProbe {
 public:
  // `attribute_service` may be null, in which case the Server is taken to be
  // current: there is nothing to ask and nothing to rescale against.
  explicit LegacySeverityProbe(
      std::shared_ptr<scada::AttributeService> attribute_service);

  // True when the Server is from before 2.6 and its severities need rescaling.
  Awaitable<bool> IsServerBefore26();

 private:
  const std::shared_ptr<scada::AttributeService> attribute_service_;
  std::optional<bool> before_2_6_;
};

// Wraps the history and monitored-item services so that every event reaching
// the Client carries a 2.6-scale severity, whichever Server sent it.
//
// This is the event boundary: live events (the subscription's event
// notifications, which feed the alarm set, the tone, the journal and Device
// watch) and history reads (the unacked-events read on connect and the event
// journal's history) all pass through these two services, so every consumer
// downstream bands on one scale. Nothing the Client sends carries a severity
// -- acknowledgement is by event id, and the *Мин. важность* filter and Device
// watch's "errors only" are applied here, on received events, not sent to the
// Server -- so there is no outgoing path to scale back down, and no event can
// be scaled twice: an event is rescaled exactly once, as it enters.
//
// Against a 2.6 Server the probe answers once and the services pass events
// through untouched. The other services are returned as they are.
DataServices RescaleLegacyEventSeverities(DataServices services);
