#include "modules/debugger/request_table_model.h"

#include "base/test/scoped_mock_clock_override.h"
#include "scada/session_service_mock.h"

#include <gtest/gtest.h>

namespace {

using Phase = scada::SessionDebugger::RequestPhase;

// The only seam the model has: it subscribes through
// SessionService::GetSessionDebugger(), so driving it means emitting events.
class FixtureSessionDebugger : public scada::SessionDebugger {
 public:
  boost::signals2::scoped_connection SubscribeRequestEvents(
      const RequestEventCallback& callback) override {
    return signal_.connect(callback);
  }

  void Emit(const RequestEvent& event) { signal_(event); }

 private:
  boost::signals2::signal<void(const RequestEvent&)> signal_;
};

class RequestTableModelClockTest : public testing::Test {
 protected:
  RequestTableModelClockTest() {
    ON_CALL(session_service_, GetSessionDebugger())
        .WillByDefault(testing::Return(&debugger_));
  }

  FixtureSessionDebugger debugger_;
  testing::NiceMock<scada::MockSessionService> session_service_;
};

// V41: the model must stamp through `scada::base::NowUtc()`, which honours
// ScopedMockClockOverride, rather than sampling std::chrono::system_clock
// directly. It sampled system_clock until 2026-09-12, which put wall-clock
// times into `debugger.png` — so the capture moved on every render, a UI change
// in it was undetectable, and the provenance record reported it stale after
// every run.
//
// Asserted as "the stamp is the mock's time", which is the property that makes
// the capture reproducible. A test that only checked the stamp was recent would
// have passed against the old code.
TEST_F(RequestTableModelClockTest, StampsRequestsFromTheOverridableClock) {
  scada::base::ScopedMockClockOverride clock;
  const scada::Time frozen = clock.Now();

  RequestTableModel model{session_service_};
  debugger_.Emit({.request_id = 1, .phase = Phase::Running, .title = "Browse"});

  ASSERT_EQ(model.GetRowCount(), 1);
  EXPECT_EQ(model.request(0).start_time, frozen);
}

// And the duration follows from the clock, so the capture can produce a
// plausible Duration column by advancing the override instead of sleeping on
// the real clock — which is what made the old durations 37/38/40 ms rather than
// a fixed number.
TEST_F(RequestTableModelClockTest, MeasuresDurationAgainstTheOverridableClock) {
  scada::base::ScopedMockClockOverride clock;

  RequestTableModel model{session_service_};
  debugger_.Emit({.request_id = 1, .phase = Phase::Running, .title = "Browse"});
  clock.Advance(std::chrono::milliseconds{37});
  debugger_.Emit(
      {.request_id = 1, .phase = Phase::Succeeded, .title = "Browse"});

  ASSERT_EQ(model.GetRowCount(), 1);
  EXPECT_EQ(model.request(0).finish_time - model.request(0).start_time,
            std::chrono::milliseconds{37});

  // Asserted on the rendered cell as well, because that is what the capture
  // shows: a duration that is right in the struct and wrong in the column
  // would leave debugger.png just as unstable.
  scada::aui::TableCell cell{.row = 0, .column_id = 3};
  model.GetCell(cell);
  EXPECT_EQ(cell.text, u"37 ms");
}

}  // namespace
