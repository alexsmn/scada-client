#include "main_window/status_bar/event_status_provider.h"

#include "scada/event.h"

#include <gtest/gtest.h>

namespace {

TEST(SeverityLevelForEventTest, BucketsBySeverityThreshold) {
  EXPECT_EQ(SeverityLevelForEvent(0), aui::SeverityLevel::kNone);
  EXPECT_EQ(SeverityLevelForEvent(scada::kSeverityWarning - 1),
            aui::SeverityLevel::kNone);
  EXPECT_EQ(SeverityLevelForEvent(scada::kSeverityWarning),
            aui::SeverityLevel::kWarning);
  EXPECT_EQ(SeverityLevelForEvent(scada::kSeverityCritical - 1),
            aui::SeverityLevel::kWarning);
  EXPECT_EQ(SeverityLevelForEvent(scada::kSeverityCritical),
            aui::SeverityLevel::kCritical);
  EXPECT_EQ(SeverityLevelForEvent(scada::kSeverityMax),
            aui::SeverityLevel::kCritical);
}

}  // namespace
