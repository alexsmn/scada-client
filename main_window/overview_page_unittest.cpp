#include "main_window/overview_page.h"

#include "aui/severity_colors.h"
#include "main_window/pages/initial_page.h"
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

  // The mockup's cockpit split: the trend dominates the top ~two thirds, the
  // alarm strip sits under it.
  const PageLayoutBlock& main = page.layout.main;
  ASSERT_EQ(main.type, PageLayoutBlock::SPLIT);
  EXPECT_TRUE(main.horz);
  EXPECT_EQ(main.pos, 65);
  ASSERT_EQ(main.left->wins.size(), 1u);
  EXPECT_EQ(main.left->wins.front(), page.GetWindow(0).id);
  EXPECT_TRUE(main.left->central);
  ASSERT_EQ(main.right->wins.size(), 1u);
  EXPECT_EQ(main.right->wins.front(), page.GetWindow(1).id);
}

// A fresh (page-less) profile lands on the Overview page under the reshell
// theme — the default-landing seed (BaseMainWindow::Init falls back to
// CreateInitialPage when the profile has no pages).
TEST(InitialPageTest, ReshellSeedsTheOverviewPage) {
  const scada::aui::SeverityTheme previous = scada::aui::GetSeverityTheme();
  scada::aui::SetSeverityTheme(scada::aui::SeverityTheme::kDark);
  const Page page = CreateInitialPage();
  scada::aui::SetSeverityTheme(previous);

  ASSERT_EQ(page.GetWindowCount(), 2);
  EXPECT_EQ(page.GetWindow(0).type, "Graph");
  EXPECT_EQ(page.GetWindow(1).type, "EventJournal");
}

}  // namespace
