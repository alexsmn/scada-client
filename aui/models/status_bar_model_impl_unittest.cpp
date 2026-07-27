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

}  // namespace
}  // namespace scada::aui
