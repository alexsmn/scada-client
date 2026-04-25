#include "export/csv/csv_export.h"

#include "aui/wt/dialog_service_impl_wt.h"
#include "profile/profile.h"

#include <gtest/gtest.h>

TEST(WtCsvExportDialogTest, UnsupportedDialogRejects) {
  DialogServiceImplWt dialog_service;
  Profile profile;

  auto result = ShowCsvExportDialog(dialog_service, profile);

  EXPECT_THROW(result.get(), std::exception);
}
