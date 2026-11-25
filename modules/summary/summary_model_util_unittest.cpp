#include "modules/summary/summary_model_util.h"

#include "base/test/test_time.h"

#include <gmock/gmock.h>

#include "base/debug_util.h"

TEST(SummaryModelUtil, CalculateSummaryModelParams) {
  auto params = CalculateSummaryModelParams(
      scada::RelativeTimeRange{TestTimeFromString("15 Nov 2004 12:45:26 UTC"),
                               TestTimeFromString("16 Nov 2004 12:45:26 UTC")},
      /*interval=*/std::chrono::minutes(30),
      /*now=*/scada::Now());

  auto expected_params = SummaryModelParams{
      TestTimeFromString("15 Nov 2004 12:30:00 UTC"),
      TestTimeFromString("16 Nov 2004 13:00:00 UTC"),
      49,
  };

  EXPECT_EQ(params, expected_params);
}

TEST(SummaryModelUtil, CalculateSummaryModelParams_DayHourly) {
  auto params = CalculateSummaryModelParams(
      scada::RelativeTimeRange{TestTimeFromString("15 Nov 2004 10:00:00 UTC"),
                               TestTimeFromString("16 Nov 2004 10:00:00 UTC")},
      /*interval=*/std::chrono::hours(1),
      /*now=*/scada::Now());

  auto expected_params = SummaryModelParams{
      TestTimeFromString("15 Nov 2004 10:00:00 UTC"),
      TestTimeFromString("16 Nov 2004 10:00:00 UTC"),
      24,
  };

  EXPECT_EQ(params, expected_params);
}

TEST(SummaryModelUtil, CalculateSummaryModelParams_PriciseBounds) {
  auto params = CalculateSummaryModelParams(
      scada::RelativeTimeRange{TestTimeFromString("15 Nov 2004 12:00:00 UTC"),
                               TestTimeFromString("16 Nov 2004 15:00:00 UTC")},
      /*interval=*/std::chrono::minutes(30),
      /*now=*/scada::Now());

  auto expected_params = SummaryModelParams{
      TestTimeFromString("15 Nov 2004 12:00:00 UTC"),
      TestTimeFromString("16 Nov 2004 15:00:00 UTC"),
      54,  // = ((24 - 12) + 15) * 2
  };

  EXPECT_EQ(params, expected_params);
}

// A period longer than the cap used to be cut with no sign of it (backlog 871);
// the params now say so, and a period that fits is not marked.
TEST(SummaryModelUtil, CalculateSummaryModelParams_MarksATruncatedPeriod) {
  const scada::Time now = scada::Time{} + std::chrono::days{20000};
  const auto month = CalculateSummaryModelParams(
      scada::RelativeTimeRange{now - std::chrono::days{30}, now},
      std::chrono::minutes{1}, now);
  EXPECT_TRUE(month.truncated);
  EXPECT_EQ(month.row_count, kMaxRowCount);

  const auto day = CalculateSummaryModelParams(
      scada::RelativeTimeRange{now - std::chrono::days{1}, now},
      std::chrono::hours{1}, now);
  EXPECT_FALSE(day.truncated);
}

// Consumption is each interval's last reading minus the last reading before
// it; the first row is measured from the interval before the period.
TEST(SummaryModelUtil, ConsumptionIsTheDifferenceOfLastReadings) {
  const std::optional<double> readings[] = {110.0, 125.0, 125.0, 160.0};
  EXPECT_THAT(ComputeConsumption(100.0, readings),
              testing::ElementsAre(10.0, 15.0, 0.0, 35.0));
}

// An interval with no reading is one where the counter did not change, so it
// consumed nothing, and the next reading is measured from the last one seen.
TEST(SummaryModelUtil, AnEmptyIntervalConsumedNothing) {
  const std::optional<double> readings[] = {110.0, std::nullopt, std::nullopt,
                                            140.0};
  EXPECT_THAT(ComputeConsumption(100.0, readings),
              testing::ElementsAre(10.0, 0.0, 0.0, 30.0));
}

// Without a reading before the period, the rows up to the first reading have
// nothing to be measured from, and stay unknown rather than reading 0.
TEST(SummaryModelUtil, ConsumptionIsUnknownUntilAReadingIsSeen) {
  const std::optional<double> readings[] = {std::nullopt, 110.0, 125.0};
  EXPECT_THAT(ComputeConsumption(std::nullopt, readings),
              testing::ElementsAre(std::nullopt, std::nullopt, 15.0));
}

// A reset counter reads lower than before; the row shows it as negative.
TEST(SummaryModelUtil, AResetShowsAsANegativeConsumption) {
  const std::optional<double> readings[] = {5.0, 8.0};
  EXPECT_THAT(ComputeConsumption(100.0, readings),
              testing::ElementsAre(-95.0, 3.0));
}
