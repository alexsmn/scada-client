#include "events/event_severity.h"

#include "base/format.h"
#include "scada/event.h"

#include <gtest/gtest.h>

namespace events {
namespace {

// Zero backlog reads as the calm state, not as a zero count.
TEST(AlarmSummaryLabelTest, ZeroBacklogIsTheCalmState) {
  EXPECT_EQ(AlarmSummaryLabel(0, 0), u"No unacknowledged events");
}

// A live backlog names the count and the highest severity's band and number.
TEST(AlarmSummaryLabelTest, BacklogNamesCountAndHighestBand) {
  EXPECT_EQ(AlarmSummaryLabel(3, scada::kSeverityCritical),
            u"Unacknowledged: 3 · highest: Critical " +
                WideFormat(scada::kSeverityCritical));
}

// A routine (band-less) severity keeps the number without inventing a band.
TEST(AlarmSummaryLabelTest, RoutineSeverityHasNoBandName) {
  const std::u16string label = AlarmSummaryLabel(1, 10);
  EXPECT_NE(label.find(u"Unacknowledged: 1"), std::u16string::npos);
  EXPECT_NE(label.find(u": 10"), std::u16string::npos);
  EXPECT_EQ(label.find(u"Critical"), std::u16string::npos);
  EXPECT_EQ(label.find(u"Warning"), std::u16string::npos);
}

}  // namespace
}  // namespace events
