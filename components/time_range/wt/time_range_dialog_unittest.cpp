#include "components/time_range/time_range_dialog.h"

#include "aui/wt/dialog_service_impl_wt.h"
#include "base/test/awaitable_test.h"
#include "profile/profile.h"

#include <gtest/gtest.h>

TEST(WtTimeRangeDialogTest, UnsupportedDialogRejects) {
  DialogServiceImplWt dialog_service;
  Profile profile;

  auto executor = std::make_shared<TestExecutor>();
  auto result = StartAwaitable(
      executor,
      ShowTimeRangeDialog(dialog_service, {.profile_ = profile,
                                           .time_range_ = TimeRange{},
                                           .time_required_ = false}));

  EXPECT_THROW(WaitResult(executor, result), std::exception);
}
