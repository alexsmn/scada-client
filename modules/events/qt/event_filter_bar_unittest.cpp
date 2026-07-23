#include "events/qt/event_filter_bar.h"

#include "base/time/time.h"
#include "scada/date_time.h"
#include "base/relative_time_range.h"

#include <gtest/gtest.h>

namespace {

// The bar offers a non-empty set of quick-pick period ranges.
TEST(EventFilterBarPeriodTest, OffersQuickPickRanges) {
  EXPECT_GE(EventPeriodRanges().size(), 2u);
}

// Every fixed preset range reflects onto its own index.
TEST(EventFilterBarPeriodTest, FixedRangeReflectsOntoItsPreset) {
  const auto& ranges = EventPeriodRanges();
  for (size_t i = 0; i < ranges.size(); ++i)
    EXPECT_EQ(EventPeriodPresetIndex(ranges[i]), static_cast<int>(i)) << i;
}

// The 15-minute and hourly interval presets are present and reflect correctly.
TEST(EventFilterBarPeriodTest, IntervalPresetsMatch) {
  EXPECT_EQ(EventPeriodPresetIndex(
                scada::RelativeTimeRange{std::chrono::minutes(15)}),
            0);
  EXPECT_EQ(
      EventPeriodPresetIndex(scada::RelativeTimeRange{std::chrono::hours(1)}),
      1);
}

// A range that matches no quick-pick (an arbitrary custom window, or an
// interval the bar does not offer) reflects as "no preset" (-1) so the selector
// does not misrepresent it.
TEST(EventFilterBarPeriodTest, UnmatchedRangeHasNoPreset) {
  scada::RelativeTimeRange custom{
      scada::Time{},
      scada::Time{} + std::chrono::hours(3)};
  EXPECT_EQ(EventPeriodPresetIndex(custom), -1);

  EXPECT_EQ(EventPeriodPresetIndex(
                scada::RelativeTimeRange{std::chrono::minutes(42)}),
            -1);
}

}  // namespace
