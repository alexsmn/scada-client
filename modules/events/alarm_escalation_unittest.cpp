#include "events/alarm_escalation.h"

#include <gtest/gtest.h>

namespace events {
namespace {

// The rungs are independent conditions, not levels of one scale, so the tests
// below assert each one alone as well as the combination — a ladder that only
// ever lights top-down would pass a test that checked the combined state only.

TEST(AlarmEscalationTest, CalmWhenNothingStands) {
  const AlarmEscalation escalation = EscalationFor(SeverityTileCounts{});
  EXPECT_FALSE(escalation.annunciating);
  EXPECT_FALSE(escalation.flooding);
  EXPECT_FALSE(escalation.escalated());
}

// The gap this ladder exists to close: one unacknowledged critical, far below
// the flood threshold, used to light nothing in the context bar at all.
TEST(AlarmEscalationTest, OneCriticalAnnunciatesWithoutFlooding) {
  const AlarmEscalation escalation =
      EscalationFor(SeverityTileCounts{.critical = 1, .unacknowledged = 1});
  EXPECT_TRUE(escalation.annunciating);
  EXPECT_FALSE(escalation.flooding);
  EXPECT_TRUE(escalation.escalated());
}

// Severity alone does not annunciate: ISA-18.2's annunciator is about a
// critical alarm nobody has taken, and warnings never light it however many.
TEST(AlarmEscalationTest, WarningsNeverAnnunciate) {
  const AlarmEscalation escalation =
      EscalationFor(SeverityTileCounts{.warning = 9, .unacknowledged = 9});
  EXPECT_FALSE(escalation.annunciating);
  EXPECT_FALSE(escalation.flooding);
}

TEST(AlarmEscalationTest, FloodNeedsNoCritical) {
  const AlarmEscalation escalation = EscalationFor(SeverityTileCounts{
      .warning = 20, .unacknowledged = kAlarmFloodThreshold + 1});
  EXPECT_FALSE(escalation.annunciating);
  EXPECT_TRUE(escalation.flooding);
}

TEST(AlarmEscalationTest, BothRungsLightTogether) {
  const AlarmEscalation escalation = EscalationFor(SeverityTileCounts{
      .critical = 3, .unacknowledged = kAlarmFloodThreshold + 1});
  EXPECT_TRUE(escalation.annunciating);
  EXPECT_TRUE(escalation.flooding);
}

// The flood boundary is the shared one, and the web client repeats the same
// value; a change here that is not made there makes the two clients escalate
// at different moments on the same plant.
TEST(AlarmEscalationTest, FloodBoundaryMatchesTheSharedThreshold) {
  EXPECT_FALSE(
      EscalationFor(SeverityTileCounts{.unacknowledged = kAlarmFloodThreshold})
          .flooding);
  EXPECT_TRUE(EscalationFor(SeverityTileCounts{.unacknowledged =
                                                   kAlarmFloodThreshold + 1})
                  .flooding);
}

}  // namespace
}  // namespace events
