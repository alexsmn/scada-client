#include "aui/wt/dialog_service_impl_wt.h"
#include "aui/prompt_dialog.h"

#include <gtest/gtest.h>

TEST(WtDialogStubTest, MessageBoxResolvesWithOk) {
  DialogServiceImplWt dialog_service;

  auto result =
      dialog_service.RunMessageBox(u"Message", u"Title", MessageBoxMode::Info);

  EXPECT_EQ(result.get(), MessageBoxResult::Ok);
}

TEST(WtDialogStubTest, FileSelectionDialogsReject) {
  DialogServiceImplWt dialog_service;

  EXPECT_THROW(dialog_service.SelectOpenFile(u"Open").get(), std::exception);
  EXPECT_THROW(dialog_service.SelectSaveFile({.title = u"Save"}).get(),
               std::exception);
}

TEST(WtDialogStubTest, PromptDialogRejects) {
  DialogServiceImplWt dialog_service;

  auto result = RunPromptDialog(dialog_service, u"Prompt", u"Title", u"");

  EXPECT_THROW(result.get(), std::exception);
}
