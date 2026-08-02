#pragma once

#include "aui/color.h"
#include "base/any_executor.h"

#include "base/any_executor_timer.h"
#include "base/time/time.h"

#include <chrono>
#include <functional>
#include <optional>
#include <string>

namespace scada {
class SessionService;
}

class LocalEvents;

// Feeds the status strip's connection cells: connected/disconnected, the ping
// round trip and the endpoint label.
//
// It is also the only thing that surfaces a stalled client. `IsConnected()`
// reports the age of an *outstanding* ping, so a growing number there means no
// ping has completed — either the link is degraded, or the client's own message
// loop stopped running (on macOS: App Nap coalescing the timer that drives the
// whole asio io_context; see services/app_nap_suppressor.h and
// docs/client/message-loop.md). Left as a bare millisecond count it reads as
// just another number, so the provider marks the pane and raises a local event
// once the round trip crosses `kPingStallThreshold`.
class SessionStatusProvider {
 public:
  using ChangeNotifier = std::function<void()>;

  // A ping round trip at or above this is reported as "no response". The
  // session pings once a second, so a few seconds means at least one ping went
  // unanswered rather than merely a slow link.
  static constexpr scada::Duration kPingStallThreshold =
      std::chrono::seconds{3};

  SessionStatusProvider(const AnyExecutor executor,
                        scada::SessionService& session_service,
                        LocalEvents& local_events)
      : session_service_{session_service},
        local_events_{local_events},
        session_poll_timer_{executor} {}

  void Init(const ChangeNotifier& change_notifier);

  // One poll tick: reports stall transitions, then refreshes the panes.
  // `Init()` arms a 1 s repeating timer over it; tests drive it directly rather
  // than standing up an executor that can service an asio timer.
  void Poll();

  std::u16string GetConnectionStateText() const;
  std::u16string GetPingText() const;
  // Severity colour for the ping cell once the round trip crosses the stall
  // threshold; unset while the session answers normally.
  std::optional<scada::aui::Color> GetPingColor() const;
  // The connected server endpoint and client build, e.g. "host:2000 · v2.6.0".
  std::u16string GetEndpointText() const;

 private:
  // The current ping round trip, or unset when there is no session.
  std::optional<scada::Duration> PingDelay() const;

  scada::SessionService& session_service_;
  LocalEvents& local_events_;

  ChangeNotifier change_notifier_;

  // Edge state, so a stall is announced once rather than on every poll tick.
  bool stall_reported_ = false;

  AnyExecutorTimer session_poll_timer_;
};
