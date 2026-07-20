#include "events/alarm_flood.h"

#include <gtest/gtest.h>

namespace events {
namespace {

TEST(AlarmFloodTest, FloodOnlyAboveThreshold) {
  EXPECT_FALSE(IsAlarmFlood(0));
  EXPECT_FALSE(IsAlarmFlood(kAlarmFloodThreshold - 1));
  EXPECT_FALSE(
      IsAlarmFlood(kAlarmFloodThreshold));  // exactly at is not a flood
  EXPECT_TRUE(IsAlarmFlood(kAlarmFloodThreshold + 1));
  EXPECT_TRUE(IsAlarmFlood(500));
}

}  // namespace
}  // namespace events
