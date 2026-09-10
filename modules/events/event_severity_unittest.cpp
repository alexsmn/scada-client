#include "events/event_severity.h"

#include "base/format.h"
#include "scada/event.h"

#include <gtest/gtest.h>

namespace events {
namespace {

// The thresholds every severity surface bands on, on the 1-1000 BaseEventType
// scale (scada::kSeverityWarning == 600, kSeverityCritical == 800; OPC UA
// Part 5 §6.4.2, ADR 0005 phase 1). This is the whole point of the header's
// "moves every severity surface at once" promise, so it is pinned here rather
// than at any one surface — the display frame carried a private copy of both
// the thresholds and this test until backlog 624.
TEST(SeverityLevelForEventTest, BandsByThreshold) {
  EXPECT_EQ(SeverityLevelForEvent(scada::kSeverityMin),
            scada::aui::SeverityLevel::kNone);
  EXPECT_EQ(SeverityLevelForEvent(scada::kSeverityNormal),
            scada::aui::SeverityLevel::kNone);
  EXPECT_EQ(SeverityLevelForEvent(scada::kSeverityWarning - 1),
            scada::aui::SeverityLevel::kNone);
  EXPECT_EQ(SeverityLevelForEvent(scada::kSeverityWarning),
            scada::aui::SeverityLevel::kWarning);
  EXPECT_EQ(SeverityLevelForEvent(scada::kSeverityCritical - 1),
            scada::aui::SeverityLevel::kWarning);
  EXPECT_EQ(SeverityLevelForEvent(scada::kSeverityCritical),
            scada::aui::SeverityLevel::kCritical);
  EXPECT_EQ(SeverityLevelForEvent(scada::kSeverityMax),
            scada::aui::SeverityLevel::kCritical);
}

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
