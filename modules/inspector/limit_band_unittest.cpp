#include "modules/inspector/limit_band.h"

#include <gtest/gtest.h>

namespace {

constexpr LimitValues kFullBands{.lolo = 10.0,
                                 .lo = 20.0,
                                 .hi = 80.0,
                                 .hihi = 90.0};

TEST(LimitBandTest, ValueInsideTheBandsIsNormal) {
  EXPECT_EQ(LimitBandFor(50.0, kFullBands), LimitBand::kNormal);
}

TEST(LimitBandTest, ClassifiesEachBand) {
  EXPECT_EQ(LimitBandFor(85.0, kFullBands), LimitBand::kHi);
  EXPECT_EQ(LimitBandFor(95.0, kFullBands), LimitBand::kHiHi);
  EXPECT_EQ(LimitBandFor(15.0, kFullBands), LimitBand::kLo);
  EXPECT_EQ(LimitBandFor(5.0, kFullBands), LimitBand::kLoLo);
}

// A value exactly on a limit breaches it: the limit is the threshold the
// process is meant to stay inside.
TEST(LimitBandTest, ValueOnTheLimitBreachesIt) {
  EXPECT_EQ(LimitBandFor(80.0, kFullBands), LimitBand::kHi);
  EXPECT_EQ(LimitBandFor(90.0, kFullBands), LimitBand::kHiHi);
  EXPECT_EQ(LimitBandFor(20.0, kFullBands), LimitBand::kLo);
  EXPECT_EQ(LimitBandFor(10.0, kFullBands), LimitBand::kLoLo);
}

// Bands the node does not configure are never reported, so a node carrying
// only warning limits cannot claim an alarm-level breach.
TEST(LimitBandTest, UnconfiguredBandsAreNeverReported) {
  constexpr LimitValues warning_only{.lo = 20.0, .hi = 80.0};
  EXPECT_EQ(LimitBandFor(1000.0, warning_only), LimitBand::kHi);
  EXPECT_EQ(LimitBandFor(-1000.0, warning_only), LimitBand::kLo);

  constexpr LimitValues none;
  EXPECT_TRUE(none.empty());
  EXPECT_EQ(LimitBandFor(1000.0, none), LimitBand::kNormal);
}

// A misconfigured node whose Hi sits above its HiHi still reports the more
// severe breach rather than the narrower one.
TEST(LimitBandTest, OverlappingBandsReportTheMoreSevere) {
  constexpr LimitValues inverted{.hi = 95.0, .hihi = 90.0};
  EXPECT_EQ(LimitBandFor(96.0, inverted), LimitBand::kHiHi);
}

}  // namespace
