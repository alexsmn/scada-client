#include "aui/models/status_bar_model_impl.h"

#include <gtest/gtest.h>

namespace scada::aui {
namespace {

// The status-bar model surfaces the unacknowledged-alarm count that the rail
// badge reads; it returns 0 with no provider and the provider's value once set.
TEST(StatusBarModelAlarmCountTest, ReportsProviderValue) {
  StatusBarModelImpl model;
  EXPECT_EQ(model.GetAlarmCount(), 0);

  int count = 3;
  model.SetAlarmCountProvider([&count] { return count; });
  EXPECT_EQ(model.GetAlarmCount(), 3);

  count = 12;
  EXPECT_EQ(model.GetAlarmCount(), 12);
}

// Panes are flagged as context-bar (who/where) or not, so the top context
// cluster can show a curated subset rather than the whole status strip.
TEST(StatusBarModelContextPaneTest, FlagsOnlyMarkedPanes) {
  StatusBarModelImpl model;
  const int plain =
      model.AddPane({.text_provider = [] { return std::u16string{}; }});
  const int context =
      model.AddPane({.text_provider = [] { return std::u16string{}; },
                     .in_context_bar = true});

  EXPECT_FALSE(model.IsContextBarPane(plain));
  EXPECT_TRUE(model.IsContextBarPane(context));
}

}  // namespace
}  // namespace scada::aui
