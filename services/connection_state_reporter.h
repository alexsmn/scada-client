#pragma once

#include "base/timer/timer.h"
#include "core/session_state_observer.h"

namespace scada {
class SessionService;
}

class LocalEvents;

struct ConnectionStateReporterContext {
  scada::SessionService& session_service_;
  LocalEvents& local_events_;
};

class ConnectionStateReporter final : private ConnectionStateReporterContext,
                                      private scada::SessionStateObserver {
 public:
  explicit ConnectionStateReporter(ConnectionStateReporterContext&& context);
  ~ConnectionStateReporter();

  ConnectionStateReporter(const ConnectionStateReporter&) = delete;
  ConnectionStateReporter& operator=(const ConnectionStateReporter&) = delete;
 
 private:
  void OnReconnectTimer();

  // scada::SessionStateObserver
  virtual void OnSessionCreated() override;
  virtual void OnSessionDeleted(const scada::Status& status) override;

  base::OneShotTimer reconnect_timer_;
  unsigned reconnect_retry_ = 0;
};
