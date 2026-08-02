#include "device_diagnostics/device_link_state.h"

#include <gtest/gtest.h>

namespace {

TEST(DeviceLinkStateTest, EnabledAndOnlineIsUp) {
  EXPECT_EQ(DeviceLinkBandFor(/*enabled=*/true, /*online=*/true),
            DeviceLinkBand::kUp);
}

TEST(DeviceLinkStateTest, EnabledButOfflineIsDown) {
  EXPECT_EQ(DeviceLinkBandFor(/*enabled=*/true, /*online=*/false),
            DeviceLinkBand::kDown);
}

// A disabled device is neither up nor down regardless of the stale online flag:
// it is not polled, so it must read neutral rather than alarm-red.
TEST(DeviceLinkStateTest, DisabledIsNeutralEvenIfOnlineFlagSet) {
  EXPECT_EQ(DeviceLinkBandFor(/*enabled=*/false, /*online=*/true),
            DeviceLinkBand::kDisabled);
  EXPECT_EQ(DeviceLinkBandFor(/*enabled=*/false, /*online=*/false),
            DeviceLinkBand::kDisabled);
}

}  // namespace
