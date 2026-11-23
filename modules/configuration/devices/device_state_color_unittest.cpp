#include "configuration/devices/device_state_color.h"

#include <gtest/gtest.h>

namespace {

TEST(DeviceStateColorTest, OnlineIsGood) {
  EXPECT_EQ(DeviceStateQuality(DeviceState::Online),
            scada::aui::Quality::kGood);
}

TEST(DeviceStateColorTest, OfflineIsBad) {
  EXPECT_EQ(DeviceStateQuality(DeviceState::Offline),
            scada::aui::Quality::kBad);
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

// Backlog 1121: Disabled and a stale value share the uncertain band's shape,
// so the badge's tooltip must name the device state; every state that gets a
// badge has a distinct, non-empty name, and Unknown (no badge) has none.
TEST(DeviceStateColorTest, EveryBadgedStateHasItsOwnLabel) {
  const std::u16string online = DeviceStateLabel(DeviceState::Online);
  const std::u16string offline = DeviceStateLabel(DeviceState::Offline);
  const std::u16string disabled = DeviceStateLabel(DeviceState::Disabled);
  EXPECT_FALSE(online.empty());
  EXPECT_FALSE(offline.empty());
  EXPECT_FALSE(disabled.empty());
  EXPECT_NE(online, offline);
  EXPECT_NE(online, disabled);
  EXPECT_NE(offline, disabled);
  EXPECT_TRUE(DeviceStateLabel(DeviceState::Unknown).empty());
}

}  // namespace
