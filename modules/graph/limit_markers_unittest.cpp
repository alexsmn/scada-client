#include "graph/limit_markers.h"

#include <gtest/gtest.h>

namespace {

// A sentinel distinct from every real limit used below, standing in for
// MetrixDataSource's kGraphUnknownValue.
constexpr double kUnset = -1e300;

TEST(LimitMarkersTest, ReturnsAllConfiguredBandsLowToHigh) {
  const std::vector<LimitMarker> markers =
      ComputeLimitMarkers(100.0, 110.0, 130.0, 140.0, kUnset);

  ASSERT_EQ(markers.size(), 4u);
  EXPECT_EQ(markers[0].kind, LimitKind::kLoLo);
  EXPECT_EQ(markers[0].value, 100.0);
  EXPECT_EQ(markers[1].kind, LimitKind::kLo);
  EXPECT_EQ(markers[1].value, 110.0);
  EXPECT_EQ(markers[2].kind, LimitKind::kHi);
  EXPECT_EQ(markers[2].value, 130.0);
  EXPECT_EQ(markers[3].kind, LimitKind::kHiHi);
  EXPECT_EQ(markers[3].value, 140.0);
}

TEST(LimitMarkersTest, OmitsUnsetBands) {
  // Only the warning bands configured; the alarm bands are unset.
  const std::vector<LimitMarker> markers =
      ComputeLimitMarkers(kUnset, 110.0, 130.0, kUnset, kUnset);

  ASSERT_EQ(markers.size(), 2u);
  EXPECT_EQ(markers[0].kind, LimitKind::kLo);
  EXPECT_EQ(markers[0].value, 110.0);
  EXPECT_EQ(markers[1].kind, LimitKind::kHi);
  EXPECT_EQ(markers[1].value, 130.0);
}

TEST(LimitMarkersTest, EmptyWhenNothingConfigured) {
  EXPECT_TRUE(
      ComputeLimitMarkers(kUnset, kUnset, kUnset, kUnset, kUnset).empty());
}

TEST(LimitMarkersTest, KeepsGenuineZeroAndNegativeLimits) {
  // 0.0 and negative values are real limits, not "unset".
  const std::vector<LimitMarker> markers =
      ComputeLimitMarkers(-40.0, -20.0, 0.0, kUnset, kUnset);

  ASSERT_EQ(markers.size(), 3u);
  EXPECT_EQ(markers[0].value, -40.0);
  EXPECT_EQ(markers[1].value, -20.0);
  EXPECT_EQ(markers[2].value, 0.0);
}

TEST(LimitMarkersTest, OuterBandsAreAlarmsInnerBandsAreWarnings) {
  EXPECT_EQ(SeverityOf(LimitKind::kLoLo), scada::aui::SeverityLevel::kCritical);
  EXPECT_EQ(SeverityOf(LimitKind::kHiHi), scada::aui::SeverityLevel::kCritical);
  EXPECT_EQ(SeverityOf(LimitKind::kLo), scada::aui::SeverityLevel::kWarning);
  EXPECT_EQ(SeverityOf(LimitKind::kHi), scada::aui::SeverityLevel::kWarning);
}

TEST(LimitMarkersTest, BandNameKeysAreEnglishAndDistinct) {
  EXPECT_EQ(LimitBandNameKey(LimitKind::kLoLo), "Alarm low");
  EXPECT_EQ(LimitBandNameKey(LimitKind::kLo), "Warning low");
  EXPECT_EQ(LimitBandNameKey(LimitKind::kHi), "Warning high");
  EXPECT_EQ(LimitBandNameKey(LimitKind::kHiHi), "Alarm high");
}

}  // namespace
