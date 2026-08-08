#include "aui/wt/dialog_service_impl_wt.h"
#include "aui/prompt_dialog.h"

#include "base/test/awaitable_test.h"

#include <gtest/gtest.h>

TEST(WtDialogStubTest, MessageBoxResolvesWithOk) {
  TestExecutor executor;
  DialogServiceImplWt dialog_service;

  auto result =
      dialog_service.RunMessageBox(u"Message", u"Title", MessageBoxMode::Info);

  EXPECT_EQ(WaitAwaitable(executor, std::move(result)), MessageBoxResult::Ok);
}

TEST(WtDialogStubTest, FileSelectionDialogsReject) {
  TestExecutor executor;
  DialogServiceImplWt dialog_service;

  EXPECT_THROW(WaitAwaitable(executor, dialog_service.SelectOpenFile(u"Open")),
               std::exception);
  EXPECT_THROW(
      WaitAwaitable(executor, dialog_service.SelectSaveFile({.title = u"Save"})),
      std::exception);
}

TEST(WtDialogStubTest, PromptDialogRejects) {
  TestExecutor executor;
  DialogServiceImplWt dialog_service;

  auto result = RunPromptDialog(dialog_service, u"Prompt", u"Title", u"");

  EXPECT_THROW(WaitAwaitable(executor, std::move(result)), std::exception);
}
