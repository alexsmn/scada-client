#include "main_window/status_bar/display_runtime_status_provider.h"

#include <gtest/gtest.h>

namespace {

TEST(DisplayRuntimeStatusProviderTest, IsEmptyAndUncolouredWithNoNotice) {
  DisplaySelectionRegistry registry;
  DisplayRuntimeStatusProvider provider{registry};

  EXPECT_TRUE(provider.GetText().empty());
  EXPECT_FALSE(provider.GetColor().has_value());
}

TEST(DisplayRuntimeStatusProviderTest, ShowsTheNoticeInTheQualityColour) {
  DisplaySelectionRegistry registry;
  DisplayRuntimeStatusProvider provider{registry};

  registry.SetRuntimeNotice(u"old runtime");

  EXPECT_EQ(provider.GetText(), u"old runtime");
  EXPECT_TRUE(provider.GetColor().has_value());
}

// Repainted from a notification, so it must be subscribed; and a notice
// published again by every display that opens repaints nothing.
TEST(DisplayRuntimeStatusProviderTest, NotifiesOnlyWhenTheNoticeChanges) {
  DisplaySelectionRegistry registry;
  DisplayRuntimeStatusProvider provider{registry};

  int notifications = 0;
  provider.Init([&notifications] { ++notifications; });

  registry.SetRuntimeNotice(u"old runtime");
  registry.SetRuntimeNotice(u"old runtime");
  EXPECT_EQ(notifications, 1);

  registry.SetRuntimeNotice({});
  EXPECT_EQ(notifications, 2);
  EXPECT_TRUE(provider.GetText().empty());
}

// The notice and the selection share the registry but not their state.
TEST(DisplayRuntimeStatusProviderTest, IsIndependentOfTheSelection) {
  DisplaySelectionRegistry registry;
  DisplayRuntimeStatusProvider provider{registry};
  const int view = 0;

  registry.SetRuntimeNotice(u"old runtime");
  registry.SetSelection(&view, u"Q1");
  registry.ClearSelection(&view);

  EXPECT_EQ(provider.GetText(), u"old runtime");
  EXPECT_TRUE(registry.label().empty());
}

}  // namespace
