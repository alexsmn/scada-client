#include "main_window/overview_page.h"

#include "profile/window_definition.h"

#include <gtest/gtest.h>

namespace {

TEST(OverviewPageTest, HasTrendAndActiveAlarmWindows) {
  Page page = MakeOverviewPage();

  ASSERT_EQ(page.GetWindowCount(), 2);
  // A dominant trend and the active-alarm table.
  EXPECT_EQ(page.GetWindow(0).type, "Graph");
  EXPECT_EQ(page.GetWindow(1).type, "EventJournal");
  // The alarm table opens in "Current" mode (unacknowledged/actionable events).
  EXPECT_NE(page.GetWindow(1).FindItem("mode"), nullptr);
}

}  // namespace
