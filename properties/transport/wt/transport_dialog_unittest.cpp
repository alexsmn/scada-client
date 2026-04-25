#include "properties/transport/transport_dialog.h"

#include "aui/wt/dialog_service_impl_wt.h"

#include <gtest/gtest.h>

TEST(WtTransportDialogTest, UnsupportedDialogRejects) {
  DialogServiceImplWt dialog_service;

  auto result =
      ShowTransportDialog(dialog_service, transport::TransportString{});

  EXPECT_THROW(result.get(), std::exception);
}
