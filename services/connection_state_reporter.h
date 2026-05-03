#pragma once

#include "base/any_executor.h"

#include "base/any_executor_timer.h"

#include <boost/signals2/connection.hpp>

namespace scada {
class SessionService;
class Status;
}  // namespace scada

class LocalEvents;

struct ConnectionStateReporterContext {
  const AnyExecutor executor_;
  scada::SessionService& session_service_;
  LocalEvents& local_events_;
};

class ConnectionStateReporter final : private ConnectionStateReporterContext {
 public:
  explicit ConnectionStateReporter(ConnectionStateReporterContext&& context);
  ~ConnectionStateReporter();

  ConnectionStateReporter(const ConnectionStateReporter&) = delete;
  ConnectionStateReporter& operator=(const ConnectionStateReporter&) = delete;

 private:
  void OnReconnectTimer();

  void OnSessionCreated();
  void OnSessionDeleted(const scada::Status& status);

  AnyExecutorTimer reconnect_timer_{executor_};
  size_t reconnect_retry_ = 0;

  const boost::signals2::scoped_connection session_state_changed_connection_;
};
