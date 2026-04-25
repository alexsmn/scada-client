#include "components/time_range/time_range_dialog.h"

#include "aui/wt/dialog_service_impl_wt.h"
#include "profile/profile.h"

#include <gtest/gtest.h>

TEST(WtTimeRangeDialogTest, UnsupportedDialogRejects) {
  DialogServiceImplWt dialog_service;
  Profile profile;

  auto result =
      ShowTimeRangeDialog(dialog_service, {.profile_ = profile,
                                           .time_range_ = TimeRange{},
                                           .time_required_ = false});

  EXPECT_THROW(result.get(), std::exception);
}
