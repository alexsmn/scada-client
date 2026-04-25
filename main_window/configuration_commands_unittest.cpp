#include "main_window/configuration_commands.h"

#include "aui/dialog_service_mock.h"
#include "base/test/test_executor.h"
#include "controller/command_registry.h"
#include "controller/selection_model.h"
#include "core/selection_command_context.h"
#include "events/local_events.h"
#include "main_window/main_window_mock.h"
#include "main_window/opened_view_interface.h"
#include "model/devices_node_ids.h"
#include "node_service/node_model_mock.h"
#include "profile/profile.h"
#include "resources/common_resources.h"
#include "scada/client.h"
#include "scada/method_service_mock.h"
#include "scada/session_service_mock.h"
#include "services/task_manager_mock.h"
#include "timed_data/timed_data_service_fake.h"

#include <gmock/gmock.h>

using namespace testing;

namespace {

constexpr scada::NumericId kItemNodeId = 6001;

class MockOpenedView : public OpenedViewInterface {
 public:
  MOCK_METHOD(const WindowInfo&, GetWindowInfo, (), (const override));
  MOCK_METHOD(std::u16string, GetWindowTitle, (), (const override));
  MOCK_METHOD(void, SetWindowTitle, (std::u16string_view title), (override));
  MOCK_METHOD(WindowDefinition, Save, (), (override));
  MOCK_METHOD(ContentsModel*, GetContents, (), (override));
  MOCK_METHOD(void, Select, (const scada::NodeId& node_id), (override));
  MOCK_METHOD(promise<WindowDefinition>,
              GetOpenWindowDefinition,
              (const WindowInfo* window_info),
              (const override));
};

NodeRef MakeCommandNode(scada::node scada_node) {
  auto node_model = std::make_shared<NiceMock<MockNodeModel>>();
  ON_CALL(*node_model, GetAttribute(scada::AttributeId::NodeId))
      .WillByDefault(Return(scada::NodeId{kItemNodeId, 1}));
  ON_CALL(*node_model, GetAttribute(scada::AttributeId::DisplayName))
      .WillByDefault(Return(scada::LocalizedText{u"Pump"}));
  ON_CALL(*node_model, GetScadaNode()).WillByDefault(Return(scada_node));
  return node_model;
}

}  // namespace

class ConfigurationCommandsTest : public Test {
 protected:
  ConfigurationCommandsTest()
      : scada_client_{scada::services{.method_service = &method_service_}},
        command_node_{MakeCommandNode(scada_client_.node({kItemNodeId, 1}))},
        selection_{SelectionModelContext{timed_data_service_}},
        commands_{selection_commands_,
                  executor_,
                  timed_data_service_,
                  session_service_,
                  profile_,
                  local_events_,
                  task_manager_} {
    selection_.SelectNode(command_node_);
    commands_.Register();
  }

  SelectionCommandContext MakeCommandContext() {
    return {.selection = selection_,
            .dialog_service = dialog_service_,
            .main_window = main_window_,
            .opened_view = opened_view_};
  }

  void ExecuteInterrogateCommand() {
    auto* command = selection_commands_.FindCommand(ID_DEV1_REFR);
    ASSERT_NE(command, nullptr);
    auto context = MakeCommandContext();
    command->execute_handler(context);
  }

  void PollExecutor() { executor_->Poll(); }

  std::shared_ptr<TestExecutor> executor_ = std::make_shared<TestExecutor>();
  StrictMock<scada::MockMethodService> method_service_;
  scada::client scada_client_;
  NodeRef command_node_;
  FakeTimedDataService timed_data_service_;
  BasicCommandRegistry<SelectionCommandContext> selection_commands_;
  NiceMock<scada::MockSessionService> session_service_;
  Profile profile_;
  LocalEvents local_events_;
  NiceMock<MockTaskManager> task_manager_;
  SelectionModel selection_;
  NiceMock<MockDialogService> dialog_service_;
  NiceMock<MockMainWindow> main_window_;
  NiceMock<MockOpenedView> opened_view_;
  ConfigurationCommands commands_;
};

TEST_F(ConfigurationCommandsTest, ReportsMethodCallSuccessAfterCompletion) {
  scada::StatusCallback callback;
  EXPECT_CALL(method_service_,
              Call(scada::NodeId{kItemNodeId, 1},
                   devices::id::DeviceType_Interrogate, IsEmpty(), _, _))
      .WillOnce(SaveArg<4>(&callback));

  ExecuteInterrogateCommand();
  PollExecutor();
  ASSERT_TRUE(static_cast<bool>(callback));
  EXPECT_TRUE(local_events_.events().empty());

  callback(scada::StatusCode::Good);
  PollExecutor();

  ASSERT_EQ(local_events_.events().size(), 1);
  const auto& event = *local_events_.events().front();
  EXPECT_EQ(event.severity, scada::kSeverityNormal);
  EXPECT_NE(event.message.find(u"Pump"), std::u16string::npos);
}

TEST_F(ConfigurationCommandsTest, ReportsMethodCallFailureAfterCompletion) {
  scada::StatusCallback callback;
  EXPECT_CALL(method_service_,
              Call(scada::NodeId{kItemNodeId, 1},
                   devices::id::DeviceType_Interrogate, IsEmpty(), _, _))
      .WillOnce(SaveArg<4>(&callback));

  ExecuteInterrogateCommand();
  PollExecutor();
  ASSERT_TRUE(static_cast<bool>(callback));

  callback(scada::StatusCode::Bad_WrongMethodId);
  PollExecutor();

  ASSERT_EQ(local_events_.events().size(), 1);
  const auto& event = *local_events_.events().front();
  EXPECT_EQ(event.severity, scada::kSeverityCritical);
  EXPECT_NE(event.message.find(u"Pump"), std::u16string::npos);
}
