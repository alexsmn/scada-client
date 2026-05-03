#include "components/limits/limit_dialog.h"

#include "aui/wt/dialog_service_impl_wt.h"
#include "base/test/awaitable_test.h"
#include "components/limits/limit_model.h"
#include "model/namespaces.h"
#include "scada/standard_node_ids.h"
#include "services/test/test_task_manager.h"

#include <gtest/gtest.h>

TEST(WtLimitDialogTest, ShowLimitsDialogRejectsUnsupportedDialog) {
  DialogServiceImplWt dialog_service;
  TestStorage storage{scada::NodeId{scada::id::RootFolder}};
  TestTaskManager task_manager{storage};

  TestExecutor executor;
  auto result = StartAwaitable(
      executor, ShowLimitsDialog(
                    dialog_service,
                    LimitDialogContext{.node_ = NodeRef{},
                                       .task_manager_ = task_manager}));

  EXPECT_THROW(WaitResult(executor, result), std::exception);
}
