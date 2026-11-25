#include "controller/period_comparison.h"

#include <gtest/gtest.h>

namespace {

TEST(PeriodComparisonTest, ShiftsByTheChosenWindow) {
  const scada::Duration period = std::chrono::hours(6);
  EXPECT_EQ(PeriodComparisonShift(PeriodComparison::kNone, period),
            scada::Duration::zero());
  EXPECT_EQ(PeriodComparisonShift(PeriodComparison::kDayEarlier, period),
            std::chrono::days(1));
  EXPECT_EQ(PeriodComparisonShift(PeriodComparison::kWeekEarlier, period),
            std::chrono::weeks(1));
  EXPECT_EQ(PeriodComparisonShift(PeriodComparison::kPreviousPeriod, period),
            period);
}

// A saved window reopens with the comparison it was saved with, and one this
// build does not know opens with none.
TEST(PeriodComparisonTest, TheSavedNameRoundTrips) {
  for (PeriodComparison comparison :
       {PeriodComparison::kNone, PeriodComparison::kDayEarlier,
        PeriodComparison::kWeekEarlier, PeriodComparison::kPreviousPeriod}) {
    EXPECT_EQ(ParsePeriodComparison(PeriodComparisonName(comparison)),
              comparison);
  }
  EXPECT_EQ(ParsePeriodComparison("fortnight"), PeriodComparison::kNone);
}

TEST(PeriodComparisonTest, OnlyAComparisonHasASuffix) {
  EXPECT_TRUE(PeriodComparisonSuffix(PeriodComparison::kNone).empty());
  EXPECT_FALSE(PeriodComparisonSuffix(PeriodComparison::kDayEarlier).empty());
}

}  // namespace
