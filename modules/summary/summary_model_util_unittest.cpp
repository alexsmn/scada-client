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
