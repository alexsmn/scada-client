#include "events/alarm_state.h"

#include "scada/event.h"

#include <gtest/gtest.h>

namespace events {
namespace {

scada::Event MakeCondition(bool active, bool acked) {
  scada::Event event;
  event.event_id = 1;
  event.source_node_id = scada::NodeId{7, 1};
  event.condition_id = event.source_node_id;
  event.active = active;
  event.acked = acked;
  return event;
}

// ISA-18.2's three alarm-list states each have their own label, so the events
// panel never relies on colour to tell them apart (backlog 1117).
TEST(AlarmStateTest, ConditionStatesAreDistinctAndLabelled) {
  EXPECT_EQ(AlarmStateFor(MakeCondition(true, false)),
            AlarmState::kUnackedActive);
  EXPECT_EQ(AlarmStateFor(MakeCondition(true, true)), AlarmState::kAckedActive);
  EXPECT_EQ(AlarmStateFor(MakeCondition(false, false)),
            AlarmState::kUnackedReturned);
  EXPECT_EQ(AlarmStateFor(MakeCondition(false, true)), AlarmState::kReturned);

  const std::u16string labels[] = {
      AlarmStateLabel(AlarmState::kUnackedActive),
      AlarmStateLabel(AlarmState::kAckedActive),
      AlarmStateLabel(AlarmState::kUnackedReturned),
      AlarmStateLabel(AlarmState::kReturned)};
  for (size_t i = 0; i < std::size(labels); ++i) {
    EXPECT_FALSE(labels[i].empty());
    for (size_t j = i + 1; j < std::size(labels); ++j)
      EXPECT_NE(labels[i], labels[j]);
  }
}

// A plain event has no process state: it stands until somebody reads it.
TEST(AlarmStateTest, PlainEventIsActiveUntilAcknowledged) {
  scada::Event event;
  EXPECT_EQ(AlarmStateFor(event), AlarmState::kUnackedEvent);
  EXPECT_TRUE(IsAlarmActive(event));
  event.acked = true;
  EXPECT_EQ(AlarmStateFor(event), AlarmState::kAckedEvent);
  EXPECT_FALSE(IsAlarmActive(event));
}

TEST(AlarmStateTest, ConditionIsActiveWhileItStands) {
  EXPECT_TRUE(IsAlarmActive(MakeCondition(true, true)));
  EXPECT_FALSE(IsAlarmActive(MakeCondition(false, false)));
}

}  // namespace
}  // namespace events
