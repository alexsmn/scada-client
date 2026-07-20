#include "graph/series_stats.h"

#include "scada/qualifier.h"
#include "scada/variant.h"

#include <gtest/gtest.h>

#include <vector>

namespace {

scada::DateTime At(double seconds) {
  return scada::base::Time::FromDoubleT(seconds);
}

// Builds a good-quality numeric sample at `seconds`.
scada::DataValue Good(double value, double seconds) {
  return scada::DataValue{scada::Variant{value}, scada::Qualifier{},
                          At(seconds), At(seconds)};
}

// Builds a bad-quality (failed) sample at `seconds`.
scada::DataValue Bad(double value, double seconds) {
  scada::Qualifier qualifier;
  qualifier.set_bad(true);
  return scada::DataValue{scada::Variant{value}, qualifier, At(seconds),
                          At(seconds)};
}

TEST(ComputeSeriesStatsTest, EmptyRangeIsInvalid) {
  std::vector<scada::DataValue> values;
  SeriesStats stats = ComputeSeriesStats(values, At(0), At(100));
  EXPECT_FALSE(stats.valid);
  EXPECT_EQ(stats.count, 0u);
}

TEST(ComputeSeriesStatsTest, MinMaxAverageOverGoodSamples) {
  std::vector<scada::DataValue> values{Good(10.0, 1), Good(30.0, 2),
                                       Good(20.0, 3)};
  SeriesStats stats = ComputeSeriesStats(values, At(0), At(100));
  ASSERT_TRUE(stats.valid);
  EXPECT_EQ(stats.count, 3u);
  EXPECT_DOUBLE_EQ(stats.min, 10.0);
  EXPECT_DOUBLE_EQ(stats.max, 30.0);
  EXPECT_DOUBLE_EQ(stats.average, 20.0);
}

TEST(ComputeSeriesStatsTest, ExcludesSamplesOutsideRange) {
  std::vector<scada::DataValue> values{Good(5.0, 1), Good(100.0, 5),
                                       Good(7.0, 9), Good(999.0, 50)};
  // Window [2, 10] keeps only the 100.0 and 7.0 samples.
  SeriesStats stats = ComputeSeriesStats(values, At(2), At(10));
  ASSERT_TRUE(stats.valid);
  EXPECT_EQ(stats.count, 2u);
  EXPECT_DOUBLE_EQ(stats.min, 7.0);
  EXPECT_DOUBLE_EQ(stats.max, 100.0);
  EXPECT_DOUBLE_EQ(stats.average, 53.5);
}

TEST(ComputeSeriesStatsTest, RangeBoundsAreInclusive) {
  std::vector<scada::DataValue> values{Good(1.0, 10), Good(2.0, 20)};
  SeriesStats stats = ComputeSeriesStats(values, At(10), At(20));
  ASSERT_TRUE(stats.valid);
  EXPECT_EQ(stats.count, 2u);
  EXPECT_DOUBLE_EQ(stats.min, 1.0);
  EXPECT_DOUBLE_EQ(stats.max, 2.0);
}

TEST(ComputeSeriesStatsTest, ExcludesBadQualitySamples) {
  // A bad-quality dropout to an extreme value must not skew the extremes.
  std::vector<scada::DataValue> values{Good(10.0, 1), Bad(-9999.0, 2),
                                       Good(20.0, 3)};
  SeriesStats stats = ComputeSeriesStats(values, At(0), At(100));
  ASSERT_TRUE(stats.valid);
  EXPECT_EQ(stats.count, 2u);
  EXPECT_DOUBLE_EQ(stats.min, 10.0);
  EXPECT_DOUBLE_EQ(stats.max, 20.0);
  EXPECT_DOUBLE_EQ(stats.average, 15.0);
}

TEST(ComputeSeriesStatsTest, AllBadInRangeIsInvalid) {
  std::vector<scada::DataValue> values{Bad(1.0, 1), Bad(2.0, 2)};
  SeriesStats stats = ComputeSeriesStats(values, At(0), At(100));
  EXPECT_FALSE(stats.valid);
  EXPECT_EQ(stats.count, 0u);
}

TEST(ComputeSeriesStatsTest, CoercesIntegerVariantsToDouble) {
  std::vector<scada::DataValue> values{
      scada::DataValue{scada::Variant{1}, scada::Qualifier{}, At(1), At(1)},
      scada::DataValue{scada::Variant{4}, scada::Qualifier{}, At(2), At(2)}};
  SeriesStats stats = ComputeSeriesStats(values, At(0), At(100));
  ASSERT_TRUE(stats.valid);
  EXPECT_EQ(stats.count, 2u);
  EXPECT_DOUBLE_EQ(stats.average, 2.5);
}

TEST(ComputeSeriesStatsTest, ExcludesNonNumericGoodSamples) {
  // A good-quality but empty (non-numeric) value contributes nothing.
  std::vector<scada::DataValue> values{
      Good(10.0, 1),
      scada::DataValue{scada::Variant{}, scada::Qualifier{}, At(2), At(2)},
      Good(20.0, 3)};
  SeriesStats stats = ComputeSeriesStats(values, At(0), At(100));
  ASSERT_TRUE(stats.valid);
  EXPECT_EQ(stats.count, 2u);
  EXPECT_DOUBLE_EQ(stats.average, 15.0);
}

}  // namespace
