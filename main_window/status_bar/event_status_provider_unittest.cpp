#include "main_window/status_bar/event_status_provider.h"

#include "scada/event.h"

#include <gtest/gtest.h>

namespace {

TEST(SeverityLevelForEventTest, BucketsBySeverityThreshold) {
  EXPECT_EQ(SeverityLevelForEvent(0), scada::aui::SeverityLevel::kNone);
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

}  // namespace
