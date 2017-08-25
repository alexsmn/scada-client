#pragma once

#include "core/session_state_observer.h"

namespace scada {
class SessionService;
}

class LocalEvents;

class ConnectionStateReporter : private scada::SessionStateObserver {
 public:
  explicit ConnectionStateReporter(scada::SessionService& notifier, LocalEvents& local_events);
  ~ConnectionStateReporter();

  ConnectionStateReporter(const ConnectionStateReporter&) = delete;
  ConnectionStateReporter& operator=(const ConnectionStateReporter&) = delete;
 
 private:
  // scada::SessionStateObserver
  virtual void OnSessionCreated() override;
  virtual void OnSessionDeleted(const scada::Status& status) override;

  scada::SessionService& session_service_;
  LocalEvents& local_events_;
};
