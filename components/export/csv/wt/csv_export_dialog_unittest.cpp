#include "export/csv/csv_export.h"

#include "aui/wt/dialog_service_impl_wt.h"
#include "base/test/awaitable_test.h"
#include "profile/profile.h"

#include <gtest/gtest.h>

TEST(WtCsvExportDialogTest, UnsupportedDialogRejects) {
  TestExecutor executor;
  DialogServiceImplWt dialog_service;
  Profile profile;

  auto result = ShowCsvExportDialog(dialog_service, profile);

  EXPECT_THROW(WaitAwaitable(executor, std::move(result)), std::exception);
}
