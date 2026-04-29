#include "components/limits/limit_dialog.h"

#include "aui/wt/dialog_service_impl_wt.h"
#include "components/limits/limit_model.h"
#include "model/namespaces.h"
#include "scada/standard_node_ids.h"
#include "services/test/test_task_manager.h"

#include <gtest/gtest.h>

TEST(WtLimitDialogTest, ShowLimitsDialogRejectsUnsupportedDialog) {
  DialogServiceImplWt dialog_service;
  TestStorage storage{scada::NodeId{scada::id::RootFolder}};
  TestTaskManager task_manager{storage};

  auto result = ShowLimitsDialog(
      dialog_service,
      LimitDialogContext{.node_ = NodeRef{}, .task_manager_ = task_manager});

  EXPECT_THROW(result.get(), std::exception);
}
