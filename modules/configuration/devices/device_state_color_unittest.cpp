#include "configuration/devices/device_state_color.h"

#include <gtest/gtest.h>

namespace {

TEST(DeviceStateColorTest, OnlineIsGood) {
  EXPECT_EQ(DeviceStateQuality(DeviceState::Online),
            scada::aui::Quality::kGood);
}

TEST(DeviceStateColorTest, OfflineIsBad) {
  EXPECT_EQ(DeviceStateQuality(DeviceState::Offline), scada::aui::Quality::kBad);
}

// Disabled is deliberately uncertain, not bad: a device that is not polled must
// not read as an alarm.
TEST(DeviceStateColorTest, DisabledIsUncertain) {
  EXPECT_EQ(DeviceStateQuality(DeviceState::Disabled),
            scada::aui::Quality::kUncertain);
}

TEST(DeviceStateColorTest, UnknownHasNoDot) {
  EXPECT_FALSE(DeviceStateQuality(DeviceState::Unknown).has_value());
}

}  // namespace
