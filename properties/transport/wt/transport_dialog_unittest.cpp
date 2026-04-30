#include "properties/transport/transport_dialog.h"

#include "aui/wt/dialog_service_impl_wt.h"
#include "base/test/awaitable_test.h"

#include <gtest/gtest.h>

TEST(WtTransportDialogTest, UnsupportedDialogRejects) {
  DialogServiceImplWt dialog_service;

  auto executor = std::make_shared<TestExecutor>();
  auto result = StartAwaitable(
      executor, ShowTransportDialog(dialog_service, transport::TransportString{}));

  EXPECT_THROW(WaitResult(executor, result), std::exception);
}
