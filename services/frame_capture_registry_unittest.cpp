#include "services/frame_capture_registry.h"

#include <gtest/gtest.h>

namespace {

constexpr scada::NodeId kDeviceA{9001, 1};
constexpr scada::NodeId kDeviceB{9002, 1};

TEST(FrameCaptureRegistryTest, ListsArmedDevicesInArmingOrder) {
  FrameCaptureRegistry registry;

  registry.SetArmed(kDeviceB, u"KP-02", true);
  registry.SetArmed(kDeviceA, u"KP-01", true);

  ASSERT_EQ(registry.armed().size(), 2u);
  EXPECT_EQ(registry.armed()[0].display_name, u"KP-02");
  EXPECT_EQ(registry.armed()[1].display_name, u"KP-01");
}

// A view disarms on both the mode switch and its own teardown, so the second
// call must be a no-op rather than removing someone else's entry or notifying
// again.
TEST(FrameCaptureRegistryTest, ArmingAndDisarmingAreIdempotent) {
  FrameCaptureRegistry registry;
  int changes = 0;
  auto connection = registry.SubscribeChanged([&changes] { ++changes; });

  registry.SetArmed(kDeviceA, u"KP-01", true);
  registry.SetArmed(kDeviceA, u"KP-01", true);
  EXPECT_EQ(registry.armed().size(), 1u);
  EXPECT_EQ(changes, 1);

  registry.SetArmed(kDeviceA, u"KP-01", false);
  registry.SetArmed(kDeviceA, u"KP-01", false);
  EXPECT_TRUE(registry.armed().empty());
  EXPECT_EQ(changes, 2);
}

TEST(FrameCaptureRegistryTest, DisarmingOneDeviceLeavesTheOthers) {
  FrameCaptureRegistry registry;
  registry.SetArmed(kDeviceA, u"KP-01", true);
  registry.SetArmed(kDeviceB, u"KP-02", true);

  registry.SetArmed(kDeviceA, u"KP-01", false);

  ASSERT_EQ(registry.armed().size(), 1u);
  EXPECT_EQ(registry.armed()[0].device_id, kDeviceB);
  EXPECT_FALSE(registry.IsArmed(kDeviceA));
  EXPECT_TRUE(registry.IsArmed(kDeviceB));
}

// A device armed before its display name resolved must not stay nameless in
// the status strip — "Capturing · " with nothing after it names no device.
TEST(FrameCaptureRegistryTest, ReArmingUpdatesADisplayNameThatHasResolved) {
  FrameCaptureRegistry registry;
  int changes = 0;
  auto connection = registry.SubscribeChanged([&changes] { ++changes; });

  registry.SetArmed(kDeviceA, u"", true);
  registry.SetArmed(kDeviceA, u"KP-01", true);

  ASSERT_EQ(registry.armed().size(), 1u);
  EXPECT_EQ(registry.armed()[0].display_name, u"KP-01");
  EXPECT_EQ(changes, 2);
}

}  // namespace
