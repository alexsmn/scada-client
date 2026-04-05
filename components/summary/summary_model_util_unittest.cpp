#include "components/summary/summary_model_util.h"

#include "base/test/test_time.h"

#include <gmock/gmock.h>

#include "base/debug_util-inl.h"

TEST(SummaryModelUtil, CalculateSummaryModelParams) {
  auto params = CalculateSummaryModelParams(
      TimeRange{TestTimeFromString("15 Nov 2004 12:45:26 UTC"),
                TestTimeFromString("16 Nov 2004 12:45:26 UTC")},
      /*interval=*/scada::Duration::FromMinutes(30),
      /*now=*/base::Time::Now());

  auto expected_params = SummaryModelParams{
      TestTimeFromString("15 Nov 2004 12:30:00 UTC"),
      TestTimeFromString("16 Nov 2004 13:00:00 UTC"),
      49,
  };

  EXPECT_EQ(params, expected_params);
}

TEST(SummaryModelUtil, CalculateSummaryModelParams_DayHourly) {
  auto params = CalculateSummaryModelParams(
      TimeRange{TestTimeFromString("15 Nov 2004 10:00:00 UTC"),
                TestTimeFromString("16 Nov 2004 10:00:00 UTC")},
      /*interval=*/scada::Duration::FromHours(1), /*now=*/base::Time::Now());

  auto expected_params = SummaryModelParams{
      TestTimeFromString("15 Nov 2004 10:00:00 UTC"),
      TestTimeFromString("16 Nov 2004 10:00:00 UTC"),
      24,
  };

  EXPECT_EQ(params, expected_params);
}

TEST(SummaryModelUtil, CalculateSummaryModelParams_PriciseBounds) {
  auto params = CalculateSummaryModelParams(
      TimeRange{TestTimeFromString("15 Nov 2004 12:00:00 UTC"),
                TestTimeFromString("16 Nov 2004 15:00:00 UTC")},
      /*interval=*/scada::Duration::FromMinutes(30), /*now=*/base::Time::Now());

  auto expected_params = SummaryModelParams{
      TestTimeFromString("15 Nov 2004 12:00:00 UTC"),
      TestTimeFromString("16 Nov 2004 15:00:00 UTC"),
      54,  // = ((24 - 12) + 15) * 2
  };

  EXPECT_EQ(params, expected_params);
}
