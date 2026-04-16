#include "client_application.h"

#include "aui/test/app_environment.h"
#include "base/client_paths.h"
#include "base/test/scoped_path_override.h"
#include "base/test/test_executor.h"
#include "controller/controller_registry.h"
#include "main_window/main_window.h"
#include "main_window/main_window_manager.h"
#include "main_window/opened_view.h"
#include "node_service/node_service.h"
#include "profile/profile.h"
#include "scada/monitoring_parameters.h"
#include "scada/read_value_id.h"
#include "scada/services_mock.h"
#include "services/task_manager.h"
#include "timed_data/timed_data_service.h"
#include "timed_data/timed_data_spec.h"

#include <boost/asio/io_context.hpp>
#include <gmock/gmock.h>

#include <vector>

using namespace ::testing;

namespace {

// Intentionally skip "Modus" and "Web", since they create an ActiveX control.
static constexpr std::string_view kKnownWindowTypes[] = {
    "CusTable", "Event",        "EventJournal", "Favorites", "FileSystemView",
    "Graph",    "Log",          "NewProps",     "Nodes",     "Params",
    "Struct",   "Subsystems",   "Summ",         "Table",     "TableEditor",
    "TimeVal",  "Transmission", "Users"};

Page MakeKnownWindowsPage() {
  Page page;

  for (std::string_view type : kKnownWindowTypes) {
    page.AddWindow(WindowDefinition{type});
  }

  return page;
}

}  // namespace

class ClientApplicationTest : public Test {
 public:
  ClientApplicationTest();
  ~ClientApplicationTest();

 protected:
  boost::asio::io_context io_context_;
  std::shared_ptr<Executor> executor_ = std::make_shared<TestExecutor>();
  AppEnvironment app_env_;
  scada::MockServices services_;

  base::ScopedPathOverride private_dir_override_{client::DIR_PRIVATE};

  NiceMock<MockFunction<promise<std::optional<DataServices>>(
      DataServicesContext&& services_context)>>
      login_handler_;

  ClientApplication app_{ClientApplicationContext{
      .io_context_ = io_context_,
      .executor_ = executor_,
      .login_handler_ = login_handler_.AsStdFunction()}};
};

ClientApplicationTest::ClientApplicationTest() {
  MainWindow::SetHideForTesting();

  ON_CALL(login_handler_, Call(/*services_context=*/_))
      .WillByDefault(Return(make_resolved_promise(std::optional{
          DataServices::FromUnownedServices(services_.services())})));
}

ClientApplicationTest::~ClientApplicationTest() {
  app_.Quit().get();
}

// ShutdownStack is the teardown-callback utility used by ClientApplication.
// Test it directly so regressions in its LIFO behavior are caught without
// standing up the full application.

TEST(ShutdownStackTest, EmptyStackDestroysCleanly) {
  ShutdownStack stack;
}

TEST(ShutdownStackTest, RunsPushedActionsInLifoOrder) {
  std::vector<int> order;
  {
    ShutdownStack stack;
    stack.Push([&] { order.push_back(1); });
    stack.Push([&] { order.push_back(2); });
    stack.Push([&] { order.push_back(3); });
  }
  EXPECT_THAT(order, ElementsAre(3, 2, 1));
}

TEST(ShutdownStackTest, RunsSinglePushedAction) {
  bool ran = false;
  {
    ShutdownStack stack;
    stack.Push([&] { ran = true; });
  }
  EXPECT_TRUE(ran);
}

// TODO: Enable Wt tests once `QTabWidget::removeTab` stops returning null.
#if !defined(UI_WT)

TEST_F(ClientApplicationTest, LoginFailed) {
  EXPECT_CALL(login_handler_, Call(/*services_context=*/_))
      .WillOnce(Return(make_resolved_promise(std::optional<DataServices>{})));

  EXPECT_THROW(app_.Start().get(), LoginCanceled);
}

TEST_F(ClientApplicationTest, QuitAfterCanceledLoginResolves) {
  EXPECT_CALL(login_handler_, Call(/*services_context=*/_))
      .WillOnce(Return(make_resolved_promise(std::optional<DataServices>{})));

  EXPECT_THROW(app_.Start().get(), LoginCanceled);
  EXPECT_NO_THROW(app_.Quit().get());
}

// Ensure that the initial page is created and all windows are defined.
TEST_F(ClientApplicationTest, RunWithNewProfile) {
  app_.Start().get();
}

// Ensure that the initial page is created and all windows are defined.
TEST_F(ClientApplicationTest, OpensKnownWindows) {
  {
    Profile profile;
    profile.AddPage(MakeKnownWindowsPage());
    profile.Save();
  }

  app_.Start().get();

  const auto& main_windows = app_.main_window_manager().main_windows();
  ASSERT_THAT(main_windows, SizeIs(1));
  const MainWindow& main_window = main_windows.front();
  std::vector<std::string_view> opened_view_types;
  for (const OpenedView* opened_view : main_window.opened_views()) {
    opened_view_types.emplace_back(opened_view->window_info().name);
  }
  EXPECT_THAT(opened_view_types, UnorderedElementsAreArray(kKnownWindowTypes));
}

// Ensures that Modus displays show the actual server data when the application
// starts.
TEST_F(ClientApplicationTest, DisplaysActualDataOnStart) {
  app_.Start().get();

  // ACT

  auto node_id = scada::NodeId{1, 1};
  auto initial_timestamp = scada::DateTime::Now();
  auto initial_data_value =
      scada::DataValue{"initial-value", /*qualifier=*/{},
                       /*source_timestamp=*/initial_timestamp,
                       /*server_timestamp=*/initial_timestamp};

  auto monitored_item =
      std::make_shared<StrictMock<scada::MockMonitoredItem>>();

  EXPECT_CALL(
      services_.monitored_item_service,
      CreateMonitoredItem(
          /*read_value_id=*/FieldsAre(node_id, scada::AttributeId::Value),
          /*monitoring_params=*/Property(&scada::MonitoringParameters::is_null,
                                         IsTrue())))
      .WillOnce(Return(monitored_item));

  EXPECT_CALL(*monitored_item, Subscribe(/*handler=*/_))
      .WillOnce(
          [initial_data_value](const scada::MonitoredItemHandler& handler) {
            // Set the initial value upon subscription.
            std::get<scada::DataChangeHandler>(handler)(initial_data_value);
          });

  TimedDataSpec timed_data;
  timed_data.Connect(app_.timed_data_service(), node_id);
  EXPECT_EQ(timed_data.current(), initial_data_value);
}

// Constructs its own `ClientApplication` so individual tests can inject a
// custom `module_configurator_` to observe `BuildModuleContext()`.
class ClientApplicationConfiguratorTest : public Test {
 public:
  ClientApplicationConfiguratorTest() {
    MainWindow::SetHideForTesting();
    ON_CALL(login_handler_, Call(/*services_context=*/_))
        .WillByDefault(Return(make_resolved_promise(std::optional{
            DataServices::FromUnownedServices(services_.services())})));
  }

 protected:
  boost::asio::io_context io_context_;
  std::shared_ptr<Executor> executor_ = std::make_shared<TestExecutor>();
  AppEnvironment app_env_;
  scada::MockServices services_;
  base::ScopedPathOverride private_dir_override_{client::DIR_PRIVATE};
  NiceMock<MockFunction<promise<std::optional<DataServices>>(
      DataServicesContext&& services_context)>>
      login_handler_;
};

// PostLogin's RunModuleConfigurator phase invokes module_configurator_ with
// the context returned by BuildModuleContext(). Verify the configurator is
// called, and that the references it receives point at the application's
// own modules.
TEST_F(ClientApplicationConfiguratorTest,
       InvokesConfiguratorWithPopulatedContext) {
  bool called = false;
  Profile* captured_profile = nullptr;
  NodeService* captured_node_service = nullptr;
  TaskManager* captured_task_manager = nullptr;
  TimedDataService* captured_timed_data = nullptr;
  ControllerRegistry* captured_controller_registry = nullptr;
  Executor* captured_executor = nullptr;

  auto default_configurator = MakeDefaultClientApplicationModules();
  auto capturing_configurator =
      [&, default_configurator = std::move(default_configurator)](
          ClientApplicationModuleContext& ctx) {
        called = true;
        captured_profile = &ctx.profile_;
        captured_node_service = &ctx.node_service_;
        captured_task_manager = &ctx.task_manager_;
        captured_timed_data = &ctx.timed_data_service_;
        captured_controller_registry = &ctx.controller_registry_;
        captured_executor = ctx.executor_.get();
        default_configurator(ctx);
      };

  ClientApplication app{ClientApplicationContext{
      .io_context_ = io_context_,
      .executor_ = executor_,
      .login_handler_ = login_handler_.AsStdFunction(),
      .module_configurator_ = std::move(capturing_configurator),
  }};

  app.Start().get();

  EXPECT_TRUE(called);
  EXPECT_EQ(captured_profile, &app.profile());
  EXPECT_EQ(captured_timed_data, &app.timed_data_service());
  EXPECT_EQ(captured_controller_registry, &app.controller_registry());
  EXPECT_EQ(captured_executor, executor_.get());
  EXPECT_NE(captured_node_service, nullptr);
  EXPECT_NE(captured_task_manager, nullptr);

  app.Quit().get();
}

#endif
