#pragma once

#include "base/any_executor.h"

#include "base/any_executor_timer.h"

#include <boost/signals2/connection.hpp>

#include <chrono>
#include <vector>

namespace scada {
class SessionService;
class Status;
}  // namespace scada

class LocalEvents;

struct ConnectionStateReporterContext {
  const AnyExecutor executor_;
  scada::SessionService& session_service_;
  LocalEvents& local_events_;
  // The wait before each reconnect attempt, indexed by retry count; the last
  // entry repeats. Injectable because AnyExecutorTimer runs on the real clock,
  // which a test executor's virtual time cannot advance (backlog 646).
  std::vector<Clock::duration> reconnect_delays_ = {
      std::chrono::seconds{1}, std::chrono::seconds{5}, std::chrono::seconds{30}};
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
