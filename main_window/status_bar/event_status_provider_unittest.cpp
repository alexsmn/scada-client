#include "main_window/status_bar/event_status_provider.h"

#include "aui/translation.h"
#include "base/u16format.h"
#include "events/local_events.h"
#include "events/node_event_provider_mock.h"
#include "profile/profile.h"
#include "scada/event.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace {

TEST(SeverityLevelForEventTest, BucketsBySeverityThreshold) {
  using events::SeverityLevelForEvent;
  EXPECT_EQ(SeverityLevelForEvent(0), scada::aui::SeverityLevel::kNone);
  EXPECT_EQ(SeverityLevelForEvent(scada::kSeverityWarning - 1),
            scada::aui::SeverityLevel::kNone);
  EXPECT_EQ(SeverityLevelForEvent(scada::kSeverityWarning),
            scada::aui::SeverityLevel::kWarning);
  EXPECT_EQ(SeverityLevelForEvent(scada::kSeverityCritical - 1),
            scada::aui::SeverityLevel::kWarning);
  EXPECT_EQ(SeverityLevelForEvent(scada::kSeverityCritical),
            scada::aui::SeverityLevel::kCritical);
  EXPECT_EQ(SeverityLevelForEvent(scada::kSeverityMax),
            scada::aui::SeverityLevel::kCritical);
}

// The live counts behind the KPI severity tiles. The provider is the only place
// that knows how the client's event model maps onto the tile semantics, so the
// mapping is locked here rather than left to the widget.
class EventStatusTileCountsTest : public ::testing::Test {
 protected:
  void AddAlarm(scada::EventId event_id, unsigned severity) {
    scada::Event event;
    event.event_id = event_id;
    event.severity = severity;
    node_event_provider_.unacked_events_.emplace(event_id, event);
  }

  testing::NiceMock<MockNodeEventProvider> node_event_provider_;
  LocalEvents local_events_;
  Profile profile_;
  EventStatusProvider provider_{node_event_provider_, local_events_, profile_};
};

// Backlog 889: «Важность: N» names the events panel's threshold, which hides
// rows from that panel only. The fetcher is never narrowed by it, so the
// counts beside it are not reduced either.
TEST_F(EventStatusTileCountsTest, SeverityTextNamesThePanelThreshold) {
  profile_.event_panel_severity_min = 500;
  AddAlarm(1, scada::kSeverityWarning - 1);

  EXPECT_EQ(provider_.GetSeverityText(),
            u16format(Translate("Severity: {}"), 500u));
  EXPECT_EQ(provider_.GetAlarmCount(), 1);
}

TEST_F(EventStatusTileCountsTest, EmptyAlarmSetIsAllZero) {
  EXPECT_EQ(provider_.GetTileCounts(), events::SeverityTileCounts{});
  EXPECT_EQ(provider_.GetAlarmCount(), 0);
}

TEST_F(EventStatusTileCountsTest, CountsBySeverityAndTotal) {
  AddAlarm(1, scada::kSeverityCritical);
  AddAlarm(2, scada::kSeverityCritical);
  AddAlarm(3, scada::kSeverityWarning);
  // Below the warning threshold: an unread event, but not a severity tile.
  AddAlarm(4, scada::kSeverityNormal);

  EXPECT_EQ(provider_.GetTileCounts(),
            (events::SeverityTileCounts{
                .critical = 2, .warning = 1, .unacknowledged = 4}));

  // The per-level accessors are views onto the same aggregation.
  EXPECT_EQ(provider_.GetSeverityCount(scada::aui::SeverityLevel::kCritical),
            2);
  EXPECT_EQ(provider_.GetSeverityCount(scada::aui::SeverityLevel::kWarning), 1);
  EXPECT_EQ(provider_.GetSeverityCount(scada::aui::SeverityLevel::kNone), 0);
  EXPECT_EQ(provider_.GetAlarmCount(), 4);
}

// The tiles count the client's own warnings and errors — the set the
// escalation ladder sounds on — and not its info-level notes (backlog 888).
TEST_F(EventStatusTileCountsTest, CountsTheClientsOwnAlarmsButNotItsNotes) {
  local_events_.ReportEvent(LocalEvents::SEV_ERROR, u"comms lost");
  local_events_.ReportEvent(LocalEvents::SEV_WARNING, u"reconnect failed");
  local_events_.ReportEvent(LocalEvents::SEV_INFO, u"command sent");

  EXPECT_EQ(provider_.GetTileCounts(),
            (events::SeverityTileCounts{
                .critical = 1, .warning = 1, .unacknowledged = 2}));
}

// Acknowledging drops the event from the client's event model, so the tiles
// fall back to calm — the acceptance line for backlog 2.3.
TEST_F(EventStatusTileCountsTest, CountsFallAsAlarmsAreAcknowledged) {
  AddAlarm(1, scada::kSeverityCritical);
  AddAlarm(2, scada::kSeverityWarning);
  ASSERT_EQ(provider_.GetAlarmCount(), 2);

  node_event_provider_.unacked_events_.erase(1);

  EXPECT_EQ(provider_.GetTileCounts(),
            (events::SeverityTileCounts{
                .critical = 0, .warning = 1, .unacknowledged = 1}));
}

}  // namespace
