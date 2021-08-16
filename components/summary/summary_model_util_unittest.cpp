#include "components/summary/summary_model_util.h"

#include "base/test/test_time.h"

#include <gmock/gmock.h>

#include "base/debug_util-inl.h"

TEST(SummaryModelUtil, CalculateSummaryModelParams) {
  const auto kInterval = scada::Duration::FromMinutes(30);
  EXPECT_EQ(CalculateSummaryModelParams(
                TimeRange{TestTimeFromString("15 Nov 2004 12:45:26 UTC"),
                          TestTimeFromString("16 Nov 2004 12:45:26 UTC")},
                kInterval),
            (SummaryModelParams{
                TestTimeFromString("15 Nov 2004 12:30:00 UTC"),
                TestTimeFromString("16 Nov 2004 13:00:00 UTC"),
                49,
            }));
}

TEST(SummaryModelUtil, CalculateSummaryModelParams_PriciseBounds) {
  const auto kInterval = scada::Duration::FromMinutes(30);
  EXPECT_EQ(CalculateSummaryModelParams(
                TimeRange{TestTimeFromString("15 Nov 2004 12:00:00 UTC"),
                          TestTimeFromString("16 Nov 2004 15:00:00 UTC")},
                kInterval),
            (SummaryModelParams{
                TestTimeFromString("15 Nov 2004 12:00:00 UTC"),
                TestTimeFromString("16 Nov 2004 15:30:00 UTC"),
                55,
            }));
}
