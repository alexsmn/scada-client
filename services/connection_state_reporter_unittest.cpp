#include "services/connection_state_reporter.h"

#include "base/awaitable.h"
#include "base/test/awaitable_test.h"
#include "base/test/test_executor.h"
#include "events/local_events.h"
#include "scada/session_service_mock.h"
#include "scada/status.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>
#include <thread>

using namespace std::chrono_literals;

namespace {

// Stands in for SessionService::Reconnect(): records that the coroutine BODY
// ran, which is the whole point -- a caller that only calls Reconnect() and
// drops the awaitable never gets here. A free coroutine taking a pointer, not
// a capturing lambda coroutine, so the gmock action cannot leave it reading a
// destroyed closure.
Awaitable<void> RecordReconnect(int* reconnects) {
  ++*reconnects;
  co_return;
}

class ConnectionStateReporterTest : public ::testing::Test {
 protected:
  ConnectionStateReporterTest() {
    ON_CALL(session_service_, SubscribeSessionStateChanged(testing::_))
        .WillByDefault(
            [this](const scada::SessionService::SessionStateChangedCallback&
                       callback) {
              session_state_changed_ = callback;
              return boost::signals2::scoped_connection{};
            });
    ON_CALL(session_service_, Reconnect()).WillByDefault([this] {
      return RecordReconnect(&reconnects_);
    });
  }

  // Drains until a reconnect is seen or `rounds` pass. The zero-delay timer is
  // still a real steady_timer, whose completion reaches the executor only once
  // its context has been polled; a single Drain() returned before that on
  // Linux (scada-client run 36326279025), though not on macOS.
  void DrainForReconnect(int rounds) {
    for (int i = 0; i < rounds && reconnects_ == 0; ++i) {
      Drain(executor_);
      if (reconnects_ == 0)
        std::this_thread::sleep_for(std::chrono::milliseconds{10});
    }
  }

  TestExecutor executor_;
  testing::NiceMock<scada::MockSessionService> session_service_;
  LocalEvents local_events_;
  scada::SessionService::SessionStateChangedCallback session_state_changed_;
  int reconnects_ = 0;
};

// A lost session schedules a reconnect, and when the backoff elapses the
// reconnect actually runs. It did not: OnReconnectTimer() called the
// Reconnect() coroutine and discarded it unrun, so the client announced
// "Reconnecting in N seconds" and stayed disconnected.
TEST_F(ConnectionStateReporterTest, ReconnectRunsWhenTheBackoffElapses) {
  // A zero backoff: the reconnect timer is a real steady_timer, which the
  // test executor's virtual clock cannot advance (test_executor.h, backlog
  // 646), so the delay is injected rather than waited out.
  ConnectionStateReporter reporter{{.executor_ = executor_,
                                    .session_service_ = session_service_,
                                    .local_events_ = local_events_,
                                    .reconnect_delays_ = {0s}}};
  ASSERT_TRUE(session_state_changed_);

  session_state_changed_(/*connected=*/false,
                         scada::Status{scada::StatusCode::Bad_Disconnected});
  DrainForReconnect(200);

  EXPECT_EQ(reconnects_, 1);
}

// Backlog 922: the server stopping or restarting is a lost connection, and the
// client reconnects from it. It must not read as a takeover.
TEST_F(ConnectionStateReporterTest, ServerShutdownReconnects) {
  ConnectionStateReporter reporter{{.executor_ = executor_,
                                    .session_service_ = session_service_,
                                    .local_events_ = local_events_,
                                    .reconnect_delays_ = {0s}}};
  ASSERT_TRUE(session_state_changed_);

  session_state_changed_(
      /*connected=*/false,
      scada::Status{scada::StatusCode::Bad_ServerWasShutDown});
  DrainForReconnect(200);

  EXPECT_EQ(reconnects_, 1);
  ASSERT_FALSE(local_events_.events().empty());
  EXPECT_EQ(
      local_events_.events().back()->message.text.find(u"another workstation"),
      std::u16string::npos);
}

// The server displacing this session for another logon of the same account is
// reported as such, and the client stays down: reconnecting would displace the
// other workstation in turn.
TEST_F(ConnectionStateReporterTest, TakeoverReportsItAndDoesNotReconnect) {
  ConnectionStateReporter reporter{{.executor_ = executor_,
                                    .session_service_ = session_service_,
                                    .local_events_ = local_events_,
                                    .reconnect_delays_ = {0s}}};
  ASSERT_TRUE(session_state_changed_);

  session_state_changed_(
      /*connected=*/false,
      scada::Status{scada::StatusCode::Bad_SessionForcedLogoff});
  // The same zero backoff and the same drain that ServerShutdownReconnects
  // shows is enough for a reconnect to run.
  DrainForReconnect(50);

  EXPECT_EQ(reconnects_, 0);
  ASSERT_FALSE(local_events_.events().empty());
  EXPECT_NE(
      local_events_.events().back()->message.text.find(u"another workstation"),
      std::u16string::npos);
}

}  // namespace
