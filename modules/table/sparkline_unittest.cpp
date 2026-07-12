#include "modules/table/sparkline.h"

#include <gtest/gtest.h>

#include <array>

namespace {

TEST(SparklineTest, FewerThanTwoValuesIsEmpty) {
  EXPECT_TRUE(ComputeSparklinePoints({}, 100, 20).empty());
  const std::array<double, 1> one{5.0};
  EXPECT_TRUE(ComputeSparklinePoints(one, 100, 20).empty());
}

TEST(SparklineTest, DegenerateSizeIsEmpty) {
  const std::array<double, 2> values{1.0, 2.0};
  EXPECT_TRUE(ComputeSparklinePoints(values, 0, 20).empty());
  EXPECT_TRUE(ComputeSparklinePoints(values, 100, 0).empty());
}

TEST(SparklineTest, EndpointsSpanFullWidth) {
  const std::array<double, 4> values{1.0, 2.0, 3.0, 4.0};
  const auto points = ComputeSparklinePoints(values, 90, 20);
  ASSERT_EQ(points.size(), 4u);
  EXPECT_FLOAT_EQ(points.front().x, 0.0f);
  EXPECT_FLOAT_EQ(points.back().x, 90.0f);
  EXPECT_FLOAT_EQ(points[1].x, 30.0f);
  EXPECT_FLOAT_EQ(points[2].x, 60.0f);
}

TEST(SparklineTest, MaxIsAtTopMinIsAtBottom) {
  // padding 2, height 20 -> usable band [2, 18]. Ascending series: first value
  // is the min (bottom = 18), last is the max (top = 2).
  const std::array<double, 3> values{10.0, 20.0, 30.0};
  const auto points = ComputeSparklinePoints(values, 100, 20, 2.0f);
  ASSERT_EQ(points.size(), 3u);
  EXPECT_FLOAT_EQ(points.front().y, 18.0f);  // min
  EXPECT_FLOAT_EQ(points.back().y, 2.0f);    // max
  EXPECT_FLOAT_EQ(points[1].y, 10.0f);       // midpoint value -> mid band
}

TEST(SparklineTest, NormalizesToOwnRange) {
  // A tiny absolute variation still uses the full vertical band.
  const std::array<double, 2> values{100.0, 100.2};
  const auto points = ComputeSparklinePoints(values, 100, 20, 2.0f);
  ASSERT_EQ(points.size(), 2u);
  EXPECT_FLOAT_EQ(points[0].y, 18.0f);  // min at bottom
  EXPECT_FLOAT_EQ(points[1].y, 2.0f);   // max at top
}

TEST(SparklineTest, FlatSeriesIsCentred) {
  const std::array<double, 3> values{7.0, 7.0, 7.0};
  const auto points = ComputeSparklinePoints(values, 100, 20);
  ASSERT_EQ(points.size(), 3u);
  for (const auto& point : points)
    EXPECT_FLOAT_EQ(point.y, 10.0f);  // height / 2
}

TEST(SparklineTest, StaysWithinPaddedBand) {
  const std::array<double, 5> values{3.0, -1.0, 8.0, 2.0, 5.0};
  const float padding = 3.0f;
  const auto points = ComputeSparklinePoints(values, 88, 24, padding);
  ASSERT_EQ(points.size(), 5u);
  for (const auto& point : points) {
    EXPECT_GE(point.y, padding);
    EXPECT_LE(point.y, 24.0f - padding);
  }
}

}  // namespace
