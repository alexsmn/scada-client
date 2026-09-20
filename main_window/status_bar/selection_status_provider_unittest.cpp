#include "main_window/status_bar/selection_status_provider.h"

#include <gtest/gtest.h>

namespace {

const int kView = 1;

TEST(SelectionStatusProviderTest, IsEmptyWithNothingSelected) {
  DisplaySelectionRegistry registry;
  SelectionStatusProvider provider{registry};

  EXPECT_TRUE(provider.GetText().empty());
}

TEST(SelectionStatusProviderTest, NamesTheSelection) {
  DisplaySelectionRegistry registry;
  SelectionStatusProvider provider{registry};

  registry.SetSelection(&kView, u"Q1");

  // The label is what the cell ends with; the prefix is translated, so
  // asserting on the whole string would pin the catalog rather than the cell.
  const std::u16string text = provider.GetText();
  ASSERT_FALSE(text.empty());
  EXPECT_TRUE(text.ends_with(u"Q1"));
}

TEST(SelectionStatusProviderTest, GoesBackToEmptyWhenTheSelectionClears) {
  DisplaySelectionRegistry registry;
  SelectionStatusProvider provider{registry};

  registry.SetSelection(&kView, u"Q1");
  registry.ClearSelection(&kView);

  EXPECT_TRUE(provider.GetText().empty());
}

// The pane is repainted from a notification, so the provider has to be
// subscribed to the registry rather than polled.
TEST(SelectionStatusProviderTest, NotifiesOnSelectionChange) {
  DisplaySelectionRegistry registry;
  SelectionStatusProvider provider{registry};

  int notifications = 0;
  provider.Init([&notifications] { ++notifications; });

  registry.SetSelection(&kView, u"Q1");
  EXPECT_EQ(notifications, 1);

  registry.ClearSelection(&kView);
  EXPECT_EQ(notifications, 2);
}

}  // namespace
