#include "main_window/status_bar/session_status_provider.h"

#include "aui/severity_colors.h"
#include "aui/translation.h"
#include "events/local_events.h"
#include "scada/event.h"
#include "scada/session_service_mock.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <utility>
#include <vector>

namespace {

using ::testing::DoAll;
using ::testing::Return;
using ::testing::SetArgPointee;

// Before the macOS App Nap fix, a client whose message loop had been suspended
// looked exactly like a healthy one: the ping cell showed a large millisecond
// count and nothing else changed, so an operator had no way to tell that data
// and events had stopped arriving. These lock in the stall being *said*, not
// merely counted. See docs/client/message-loop.md.
class SessionStatusStallTest : public ::testing::Test {
 protected:
  // The newest ON_CALL wins in gMock, so each call re-poses the session.
  void SetSession(bool connected, scada::Duration ping_delay) {
    ON_CALL(session_service_, IsConnected(testing::_))
        .WillByDefault(DoAll(SetArgPointee<0>(ping_delay), Return(connected)));
  }

  // The reported local events, most recent last.
  std::vector<std::pair<scada::UInt32, std::u16string>> ReportedEvents() const {
    std::vector<std::pair<scada::UInt32, std::u16string>> result;
    for (const scada::Event* event : local_events_.events())
      result.emplace_back(event->severity, event->message.text);
    return result;
  }

  static constexpr scada::Duration kHealthy = std::chrono::milliseconds{12};
  static constexpr scada::Duration kStalled = std::chrono::seconds{50};

  testing::NiceMock<scada::MockSessionService> session_service_;
  LocalEvents local_events_;
  SessionStatusProvider provider_{AnyExecutor{}, session_service_,
                                  local_events_};
};

TEST_F(SessionStatusStallTest, HealthySessionSaysNothing) {
  SetSession(/*connected=*/true, kHealthy);

  provider_.Poll();
  provider_.Poll();

  EXPECT_THAT(ReportedEvents(), testing::IsEmpty());
  EXPECT_EQ(provider_.GetPingColor(), std::nullopt);
  // gmock's HasSubstr has no char16_t overload, hence the explicit find().
  EXPECT_EQ(provider_.GetPingText().find(Translate("no response")),
            std::u16string::npos);
}

TEST_F(SessionStatusStallTest, StalledSessionIsMarkedInThePane) {
  SetSession(/*connected=*/true, kStalled);

  EXPECT_NE(provider_.GetPingText().find(Translate("no response")),
            std::u16string::npos);
  // Colour is an additional cue only: it resolves to nothing under the legacy
  // severity theme, which is why the marker above has to be in the text.
  EXPECT_EQ(provider_.GetPingColor(),
            scada::aui::SeverityColor(scada::aui::SeverityLevel::kWarning));
}

TEST_F(SessionStatusStallTest, StallIsAnnouncedOnceAndRecoveryOnce) {
  SetSession(/*connected=*/true, kStalled);
  provider_.Poll();
  provider_.Poll();
  provider_.Poll();

  ASSERT_THAT(ReportedEvents(), testing::SizeIs(1));
  EXPECT_EQ(ReportedEvents().front().first, scada::kSeverityWarning);

  SetSession(/*connected=*/true, kHealthy);
  provider_.Poll();
  provider_.Poll();

  ASSERT_THAT(ReportedEvents(), testing::SizeIs(2));
  EXPECT_EQ(ReportedEvents().back().second,
            Translate("The server is answering again."));

  // A second stall is a new episode and is announced again.
  SetSession(/*connected=*/true, kStalled);
  provider_.Poll();

  EXPECT_THAT(ReportedEvents(), testing::SizeIs(3));
}

// Losing the session entirely is `ConnectionStateReporter`'s message, not this
// provider's: a disconnect must not be dressed up as a recovery.
TEST_F(SessionStatusStallTest, DisconnectDoesNotReportRecovery) {
  SetSession(/*connected=*/true, kStalled);
  provider_.Poll();
  ASSERT_THAT(ReportedEvents(), testing::SizeIs(1));

  SetSession(/*connected=*/false, scada::Duration::zero());
  provider_.Poll();
  provider_.Poll();

  EXPECT_THAT(ReportedEvents(), testing::SizeIs(1));
  EXPECT_EQ(provider_.GetPingColor(), std::nullopt);
}

}  // namespace
