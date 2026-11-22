#include "selection_edit/selection_edit_module.h"

#include "aui/dialog_service_mock.h"
#include "base/test/awaitable_test.h"
#include "base/test/test_executor.h"
#include "common/node_state.h"
#include "controller/command_registry.h"
#include "controller/command_ui_registry.h"
#include "controller/selection_model.h"
#include "core/selection_command_context.h"
#include "main_window/main_window_mock.h"
#include "main_window/opened_view/opened_view_command_registry.h"
#include "node_service/test/fake_node_service.h"
#include "resources/common_resources.h"
#include "scada/access_rights.h"
#include "scada/session_service_mock.h"
#include "services/task_manager_mock.h"
#include "test/fake_opened_view.h"
#include "timed_data/timed_data_service_fake.h"

#include <gmock/gmock.h>

using namespace testing;

namespace {

constexpr scada::NumericId kNodeId = 7001;

Awaitable<MessageBoxResult> AnswerWith(MessageBoxResult result) {
  co_return result;
}

class SelectionEditModuleTest : public Test {
 protected:
  SelectionEditModuleTest() {
    node_ = node_service_.Add(scada::NodeState{
        .node_id = scada::NodeId{kNodeId, 1},
        .attributes = {.display_name = scada::LocalizedText{u"Feeder 1"}}});
    selection_.SelectNode(node_);
    // An engineer, who may delete configuration nodes.
    ON_CALL(session_service_, GetAccessRights())
        .WillByDefault(Return(static_cast<std::uint32_t>(
            1u << static_cast<unsigned>(scada::AccessRight::kConfigure))));
  }

  void ExecuteDelete() {
    auto* command = selection_commands_.FindCommand(ID_DELETE);
    ASSERT_NE(command, nullptr);
    command->execute_handler(
        SelectionCommandContext{.selection = selection_,
                                .dialog_service = dialog_service_,
                                .main_window = main_window_,
                                .opened_view = opened_view_});
    Drain(executor_);
  }

  TestExecutor executor_;
  FakeNodeService node_service_;
  NodeRef node_;
  FakeTimedDataService timed_data_service_;
  SelectionModel selection_{SelectionModelContext{timed_data_service_}};
  NiceMock<scada::MockSessionService> session_service_;
  NiceMock<MockTaskManager> task_manager_;
  BasicCommandRegistry<SelectionCommandContext> selection_commands_;
  UiCommandRegistry ui_command_registry_;
  OpenedViewCommandRegistry opened_view_commands_;
  NiceMock<MockDialogService> dialog_service_;
  NiceMock<MockMainWindow> main_window_;
  FakeOpenedView opened_view_;
  SelectionEditModule module_{SelectionEditModuleContext{
      .executor_ = executor_,
      .session_service_ = session_service_,
      .node_service_ = node_service_,
      .task_manager_ = task_manager_,
      .selection_commands_ = selection_commands_,
      .ui_command_registry_ = ui_command_registry_,
      .opened_view_commands_ = opened_view_commands_}};
};

// Delete is bound to the Delete key, so the question must default to No:
// Delete then Enter would otherwise remove the node and everything under it
// (backlog 1099). Declining deletes nothing.
TEST_F(SelectionEditModuleTest, DeleteAsksWithNoAsDefaultAndDeclineKeeps) {
  EXPECT_CALL(dialog_service_,
              RunMessageBox(_, _, MessageBoxMode::QuestionYesNoDefaultNo))
      .WillOnce([](std::u16string_view, std::u16string_view, MessageBoxMode) {
        return AnswerWith(MessageBoxResult::No);
      });
  EXPECT_CALL(task_manager_, PostDeleteTask(_)).Times(0);

  ExecuteDelete();
}

TEST_F(SelectionEditModuleTest, DeleteConfirmedPostsTheDelete) {
  EXPECT_CALL(dialog_service_,
              RunMessageBox(_, _, MessageBoxMode::QuestionYesNoDefaultNo))
      .WillOnce([](std::u16string_view, std::u16string_view, MessageBoxMode) {
        return AnswerWith(MessageBoxResult::Yes);
      });
  EXPECT_CALL(task_manager_, PostDeleteTask(scada::NodeId{kNodeId, 1}));

  ExecuteDelete();
}

}  // namespace
