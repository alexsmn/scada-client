#pragma once

#include "base/lifetime.h"
#include "scada/event.h"

#include <boost/signals2/signal.hpp>
#include <memory>
#include <string>
#include <vector>

// Windows.h #defines ReportEvent to ReportEventA/W. Undo it.
#ifdef ReportEvent
#undef ReportEvent
#endif

// The Client's own messages: events this workstation raises itself rather
// than receives from the server. Only unacknowledged ones are kept;
// acknowledging one deletes it. Owned by `ClientApplication` for the life of
// the process, so they survive a sign-out (backlog 1072).
class LocalEvents {
 public:
  // TODO: Use `scada::EventSeverity` instead of `Severity`.
  enum Severity { SEV_INFO, SEV_WARNING, SEV_ERROR };

  LocalEvents();
  ~LocalEvents();

  LocalEvents(const LocalEvents&) = delete;
  LocalEvents& operator=(const LocalEvents&) = delete;

  using Events = std::vector<scada::Event*>;
  const Events& events() const SCADA_LIFETIME_BOUND { return events_; }

  void ReportEvent(Severity severity, const scada::LocalizedText& message);

  void AcknowledgeEvent(scada::EventId event_id);
  void AcknowledgeAll();

  using EventSignal = boost::signals2::signal<void(const scada::Event&)>;

  EventSignal& event_signal() SCADA_LIFETIME_BOUND { return event_signal_; }

  // Expires when this object is destroyed. For work that can outlive it -- a
  // device answering a control Cancel while the Client shuts down -- to check
  // before it reports here. A sign-out does not destroy it: the application
  // owns it across session rebuilds (backlog 1072).
  std::weak_ptr<const void> lifetime() const { return lifetime_; }

 private:
  Events::iterator FindEvent(scada::EventId event_id);

  static scada::EventSeverity SeverityToEvent(Severity severity);

  scada::EventId next_event_id_ = 1;

  Events events_;

  boost::signals2::signal<void(const scada::Event&)> event_signal_;

  const std::shared_ptr<const void> lifetime_ = std::make_shared<int>(0);
};
