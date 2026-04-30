#include "components/write/write_dialog.h"

#include "aui/wt/dialog_service_impl_wt.h"
#include "base/test/awaitable_test.h"
#include "profile/profile.h"
#include "scada/standard_node_ids.h"
#include "timed_data/timed_data_service_mock.h"

#include <gtest/gtest.h>

TEST(WtWriteDialogTest, ExecuteWriteDialogRejectsUnsupportedDialog) {
  DialogServiceImplWt dialog_service;
  testing::NiceMock<MockTimedDataService> timed_data_service;
  Profile profile;

  auto executor = std::make_shared<TestExecutor>();
  auto result = StartAwaitable(
      executor, ExecuteWriteDialog(
                    dialog_service,
                    WriteContext{
                        .timed_data_service_ = timed_data_service,
                        .node_id_ = scada::NodeId{scada::id::RootFolder},
                        .profile_ = profile,
                        .manual_ = true}));

  EXPECT_THROW(WaitResult(executor, result), std::exception);
}
