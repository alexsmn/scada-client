#include "base/time/time_wire_codec.h"
#include "modules/events/event_timeline.h"

#include "scada/event.h"

#include <gtest/gtest.h>

namespace events {
namespace {

scada::Time At(int second) {
  return scada::base::DecodeDoubleT(1'700'000'000.0 + second);
}

// A pending event always states that it is still waiting, so the section never
// leaves the operator wondering where it stands.
TEST(EventTimelineTest, PendingEventEndsAwaitingAcknowledgement) {
  scada::Event event;
  event.time = At(0);

  const std::vector<EventTimelineEntry> timeline = BuildEventTimeline(event);
  ASSERT_EQ(timeline.size(), 2u);
  EXPECT_EQ(timeline[0].step, EventTimelineStep::kRaised);
  EXPECT_EQ(timeline[0].time, At(0));
  EXPECT_EQ(timeline[1].step, EventTimelineStep::kAwaitingAcknowledgement);
  EXPECT_TRUE(scada::IsNull(timeline[1].time));
}

TEST(EventTimelineTest, AcknowledgedEventEndsAcknowledged) {
  scada::Event event;
  event.time = At(0);
  event.acked = true;
  event.acknowledged_time = At(30);

  const std::vector<EventTimelineEntry> timeline = BuildEventTimeline(event);
  ASSERT_EQ(timeline.size(), 2u);
  EXPECT_EQ(timeline[1].step, EventTimelineStep::kAcknowledged);
  EXPECT_EQ(timeline[1].time, At(30));
}

// A delivery delay is worth seeing, so a differing receive time earns a row.
TEST(EventTimelineTest, DelayedDeliveryReportsTheReceiveStep) {
  scada::Event event;
  event.time = At(0);
  event.receive_time = At(5);

  const std::vector<EventTimelineEntry> timeline = BuildEventTimeline(event);
  ASSERT_EQ(timeline.size(), 3u);
  EXPECT_EQ(timeline[1].step, EventTimelineStep::kReceived);
  EXPECT_EQ(timeline[1].time, At(5));
}

// Immediate delivery adds no information, so it adds no row.
TEST(EventTimelineTest, ImmediateDeliveryOmitsTheReceiveStep) {
  scada::Event event;
  event.time = At(0);
  event.receive_time = At(0);

  const std::vector<EventTimelineEntry> timeline = BuildEventTimeline(event);
  ASSERT_EQ(timeline.size(), 2u);
  EXPECT_EQ(timeline[1].step, EventTimelineStep::kAwaitingAcknowledgement);
}

// An event the server has not processed yet carries a null receive time.
TEST(EventTimelineTest, UnprocessedEventOmitsTheReceiveStep) {
  scada::Event event;
  event.time = At(0);

  const std::vector<EventTimelineEntry> timeline = BuildEventTimeline(event);
  ASSERT_EQ(timeline.size(), 2u);
  EXPECT_EQ(timeline[1].step, EventTimelineStep::kAwaitingAcknowledgement);
}

// Every step has operator-facing text.
TEST(EventTimelineTest, EveryStepHasText) {
  for (EventTimelineStep step :
       {EventTimelineStep::kRaised, EventTimelineStep::kReceived,
        EventTimelineStep::kAcknowledged,
        EventTimelineStep::kAwaitingAcknowledgement}) {
    EXPECT_FALSE(EventTimelineStepText(step).empty());
  }
}

}  // namespace
}  // namespace events
