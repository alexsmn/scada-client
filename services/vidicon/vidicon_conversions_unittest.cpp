#include "services/vidicon/vidicon_conversions.h"

#include <gmock/gmock.h>

using namespace testing;

namespace vidicon {

TEST(VidiconConversions, ToDATE) {
  EXPECT_EQ(ToDATE(scada::DateTime{}), 0);
  // TODO: Add more specific times.
}

TEST(VidiconConversions, ToOpcQuality) {
  EXPECT_EQ(ToOpcQuality(scada::Qualifier{}), 0xC0 /*OPC_QUALITY_GOOD*/);
  // TODO: Add `ToOpcQuality` tests.
}

}  // namespace vidicon