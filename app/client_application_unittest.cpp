#include "client_application.h"

#include "address_space/address_space_impl3.h"
#include "address_space/address_space_util.h"
#include "address_space/node_utils.h"
#include "address_space/object.h"
#include "address_space/variable.h"
#include "aui/qt/message_loop_qt.h"
#include "aui/test/app_environment.h"
#include "base/async_completion.h"
#include "base/awaitable.h"
#include "base/client_paths.h"
#include "base/test/scoped_path_override.h"
#include "controller/controller_registry.h"
#include "main_window/main_window.h"
#include "main_window/main_window_manager.h"
#include "main_window/opened_view.h"
#include "node_service/node_service.h"
#include "profile/profile.h"
#include "scada/monitoring_parameters.h"
#include "scada/read_value_id.h"
#include "scada/services_mock.h"
#include "node_service/v1/test/test_node_service.h"
#include "services/task_manager.h"
#include "timed_data/base_timed_data.h"
#include "timed_data/timed_data_service.h"
#include "timed_data/timed_data_spec.h"

#include <boost/asio/io_context.hpp>
#include <gmock/gmock.h>
#include <QApplication>
#include <QElapsedTimer>
#include <QEventLoop>

#include <optional>
#include <stdexcept>
#include <vector>

using namespace ::testing;
using namespace std::chrono_literals;

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

std::shared_ptr<NodeService> MakeClientTestNodeService(
    AddressSpaceImpl3& address_space) {
  address_space.AddStaticNode<scada::GenericVariable>(
      scada::NodeId{1, 1},
      "TestNode",
      u"TestNode",
      scada::AsDataType(*address_space.GetNode(scada::id::String)));
  scada::AddReference(address_space,
                      scada::id::HasTypeDefinition,
                      scada::NodeId{1, 1},
                      scada::id::BaseVariableType);
  scada::AddReference(address_space,
                      scada::id::Organizes,
                      scada::id::ObjectsFolder,
                      scada::NodeId{1, 1});
  return v1::CreateTestNodeService(address_space);
}

ClientApplicationModuleConfigurator MakeUnitTestModules() {
  auto modules = ClientApplicationModules{};
  modules.modus = false;
  modules.vidicon = false;
  return MakeDefaultClientApplicationModules(modules);
}

void PumpClientApplicationTestEvents(boost::asio::io_context& io_context,
                                     QEventLoop::ProcessEventsFlags flags =
                                         QEventLoop::WaitForMoreEvents,
                                     int max_time_ms = -1) {
  io_context.poll();
  if (max_time_ms >= 0) {
    QApplication::processEvents(flags, max_time_ms);
  } else {
    QApplication::processEvents(flags);
  }
}

template <class T>
struct AwaitableResult {
  std::optional<T> value;
  std::exception_ptr error;
  bool done = false;
};

template <>
struct AwaitableResult<void> {
  std::exception_ptr error;
  bool done = false;
};

template <class T>
std::shared_ptr<AwaitableResult<T>> StartClientApplicationAwaitable(
    AnyExecutor executor,
    Awaitable<T> awaitable) {
  auto result = std::make_shared<AwaitableResult<T>>();
  CoSpawn(std::move(executor),
          [result, awaitable = std::move(awaitable)]() mutable
              -> Awaitable<void> {
            try {
              if constexpr (std::is_void_v<T>) {
                co_await std::move(awaitable);
              } else {
                result->value.emplace(co_await std::move(awaitable));
              }
            } catch (...) {
              result->error = std::current_exception();
            }
            result->done = true;
          });
  return result;
}

template <class T>
bool IsReady(const std::shared_ptr<AwaitableResult<T>>& pending) {
  return pending->done;
}

template <class T>
T WaitForClientApplicationAwaitable(
    boost::asio::io_context& io_context,
    const std::shared_ptr<AwaitableResult<T>>& pending) {
  while (!IsReady(pending)) {
    PumpClientApplicationTestEvents(io_context);
  }

  if (pending->error) {
    std::rethrow_exception(pending->error);
  }
  if constexpr (!std::is_void_v<T>) {
    return std::move(*pending->value);
  }
}

class FixedValueTimedData final : public BaseTimedData {
 public:
  explicit FixedValueTimedData(scada::DataValue value) {
    UpdateCurrent(value);
  }

  std::string GetFormula(bool aliases) const override { return "fixed"; }
  scada::LocalizedText GetTitle() const override { return u"fixed"; }
};

class FixedValueTimedDataService final : public TimedDataService {
 public:
  FixedValueTimedDataService(scada::NodeId node_id, scada::DataValue value)
      : node_id_{std::move(node_id)},
        timed_data_{std::make_shared<FixedValueTimedData>(std::move(value))} {}

  std::shared_ptr<TimedData> GetNodeTimedData(
      const scada::NodeId& node_id,
      const scada::AggregateFilter& aggregation) override {
    return node_id == node_id_ ? timed_data_ : nullptr;
  }

  std::shared_ptr<TimedData> GetFormulaTimedData(
      std::string_view formula,
      const scada::AggregateFilter& aggregation) override {
    return nullptr;
  }

 private:
  const scada::NodeId node_id_;
  const std::shared_ptr<TimedData> timed_data_;
};

}  // namespace

class ClientApplicationTestBase : public Test {
 public:
  ClientApplicationTestBase();

 protected:
  // Coroutine startup exceptions are captured by the spawned awaitable and
  // rethrown here after pumping the Qt/asio loops.
  template <class T>
  T Wait(Awaitable<T> pending) {
    return WaitForClientApplicationAwaitable(
        io_context_,
        StartClientApplicationAwaitable(executor_, std::move(pending)));
  }

  void Wait(Awaitable<void> pending) {
    WaitForClientApplicationAwaitable(
        io_context_,
        StartClientApplicationAwaitable(executor_, std::move(pending)));
  }

  template <class Predicate>
  bool WaitUntil(Predicate&& predicate, int timeout_ms = 5000) {
    QElapsedTimer timer;
    timer.start();
    while (!predicate()) {
      if (timer.elapsed() >= timeout_ms) {
        return false;
      }
      PumpClientApplicationTestEvents(io_context_, QEventLoop::AllEvents, 50);
    }
    return true;
  }

  boost::asio::io_context io_context_;
  AppEnvironment app_env_;
  AnyExecutor executor_ = MakeAnyExecutor(std::make_shared<MessageLoopQt>());
  AddressSpaceImpl3 address_space_;
  std::shared_ptr<NodeService> node_service_override_ =
      MakeClientTestNodeService(address_space_);
  scada::MockServices services_;

  base::ScopedPathOverride private_dir_override_{client::DIR_PRIVATE};

  NiceMock<MockFunction<Awaitable<std::optional<DataServices>>(
      DataServicesContext&& services_context)>>
      login_handler_;
};

class ClientApplicationTest : public ClientApplicationTestBase {
 public:
  ClientApplicationTest();
  ~ClientApplicationTest();

 protected:
  void StartApp() { Wait(app_.Start()); }

  ClientApplication app_{ClientApplicationContext{
      .io_context_ = io_context_,
      .executor_ = executor_,
      .login_handler_ = login_handler_.AsStdFunction(),
      .node_service_override_ = node_service_override_,
      .module_configurator_ = MakeUnitTestModules()}};
};

ClientApplicationTestBase::ClientApplicationTestBase() {
  MainWindow::SetHideForTesting();

  ON_CALL(login_handler_, Call(/*services_context=*/_))
      .WillByDefault(Invoke([this](DataServicesContext&&)
                                -> Awaitable<std::optional<DataServices>> {
        co_return std::optional{
            DataServices::FromUnownedServices(services_.services())};
      }));
}

ClientApplicationTest::ClientApplicationTest() = default;

ClientApplicationTest::~ClientApplicationTest() {
  Wait(app_.Quit());
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
      .WillOnce(Invoke([](DataServicesContext&&)
                           -> Awaitable<std::optional<DataServices>> {
        co_return std::optional<DataServices>{};
      }));

  auto started = app_.Start();
  EXPECT_THROW(Wait(std::move(started)), LoginCanceled);
}

TEST_F(ClientApplicationTest, QuitAfterCanceledLoginResolves) {
  EXPECT_CALL(login_handler_, Call(/*services_context=*/_))
      .WillOnce(Invoke([](DataServicesContext&&)
                           -> Awaitable<std::optional<DataServices>> {
        co_return std::optional<DataServices>{};
      }));

  auto started = app_.Start();
  EXPECT_THROW(Wait(std::move(started)), LoginCanceled);
  EXPECT_NO_THROW(Wait(app_.Quit()));
}

TEST_F(ClientApplicationTest, RunWaitsUntilQuitCompletes) {
  auto running = app_.Run();
  auto running_result =
      StartClientApplicationAwaitable(executor_, std::move(running));
  EXPECT_FALSE(IsReady(running_result));

  Wait(app_.Quit());
  WaitForClientApplicationAwaitable(io_context_, running_result);

  EXPECT_NO_THROW(Wait(app_.Run()));
}

// Start() is coroutine-driven internally. Verify that a rejection from the
// login handler propagates out of the returned awaitable so upstream error
// handling still fires.
TEST_F(ClientApplicationTest, LoginHandlerRejectionPropagates) {
  struct MyError : std::runtime_error {
    MyError() : std::runtime_error{"boom"} {}
  };

  EXPECT_CALL(login_handler_, Call(/*services_context=*/_))
      .WillOnce(Invoke([](DataServicesContext&&)
                           -> Awaitable<std::optional<DataServices>> {
        throw MyError{};
        co_return std::optional<DataServices>{};
      }));

  EXPECT_THROW(Wait(app_.Start()), MyError);
}

// PostLogin depends on login_handler_ resolving first. The coroutine-based
// Start() must still preserve that order: no module should exist before
// OnLoginCompleted runs. Verified by delaying the login coroutine and
// checking that app_.Start() stays pending until login resumes.
TEST_F(ClientApplicationTest, StartWaitsForLoginBeforePostLogin) {
  base::AsyncCompletion pending_login{executor_};
  EXPECT_CALL(login_handler_, Call(/*services_context=*/_))
      .WillOnce(Invoke([&](DataServicesContext&&)
                           -> Awaitable<std::optional<DataServices>> {
        co_await pending_login.Wait();
        co_return std::optional{
            DataServices::FromUnownedServices(services_.services())};
      }));

  auto started = StartClientApplicationAwaitable(executor_, app_.Start());

  // Pump the loop a few times — Start() must not resolve while login is
  // still pending.
  for (int i = 0; i < 5 && !IsReady(started); ++i) {
    PumpClientApplicationTestEvents(io_context_, QEventLoop::AllEvents, 10);
  }
  EXPECT_FALSE(IsReady(started));

  // Resolve the login and verify Start() now completes.
  pending_login.Complete();
  WaitForClientApplicationAwaitable(io_context_, started);
}

// Ensure that the initial page is created and all windows are defined.
TEST_F(ClientApplicationTest, RunWithNewProfile) {
  StartApp();
}

// Ensure that the initial page is created and all windows are defined.
TEST_F(ClientApplicationTest, OpensKnownWindows) {
  {
    Profile profile;
    profile.AddPage(MakeKnownWindowsPage());
    profile.Save();
  }

  StartApp();

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
TEST_F(ClientApplicationTestBase, DisplaysActualDataOnStart) {
  auto node_id = scada::NodeId{1, 1};
  auto initial_timestamp = scada::DateTime::Now();
  auto initial_data_value =
      scada::DataValue{"initial-value", /*qualifier=*/{},
                       /*source_timestamp=*/initial_timestamp,
                       /*server_timestamp=*/initial_timestamp};

  ClientApplication app{ClientApplicationContext{
      .io_context_ = io_context_,
      .executor_ = executor_,
      .login_handler_ = login_handler_.AsStdFunction(),
      .timed_data_service_override_ =
          std::make_unique<FixedValueTimedDataService>(node_id,
                                                       initial_data_value),
      .node_service_override_ = node_service_override_,
      .module_configurator_ = MakeUnitTestModules()}};

  Wait(app.Start());

  TimedDataSpec timed_data;
  timed_data.Connect(app.timed_data_service(), node_id);
  ASSERT_TRUE(WaitUntil([&] { return timed_data.current() == initial_data_value; }));
  EXPECT_EQ(timed_data.current(), initial_data_value);

  Wait(app.Quit());
}

// Constructs its own `ClientApplication` so individual tests can inject a
// custom `module_configurator_` to observe `BuildModuleContext()`.
class ClientApplicationConfiguratorTest : public Test {
 public:
  ClientApplicationConfiguratorTest() {
    MainWindow::SetHideForTesting();
    ON_CALL(login_handler_, Call(/*services_context=*/_))
        .WillByDefault(Invoke([this](DataServicesContext&&)
                                  -> Awaitable<std::optional<DataServices>> {
          co_return std::optional{
              DataServices::FromUnownedServices(services_.services())};
        }));
  }

 protected:
  template <class T>
  T Wait(Awaitable<T> pending) {
    return WaitForClientApplicationAwaitable(
        io_context_,
        StartClientApplicationAwaitable(executor_, std::move(pending)));
  }

  void Wait(Awaitable<void> pending) {
    WaitForClientApplicationAwaitable(
        io_context_,
        StartClientApplicationAwaitable(executor_, std::move(pending)));
  }

  boost::asio::io_context io_context_;
  AppEnvironment app_env_;
  AnyExecutor executor_ = MakeAnyExecutor(std::make_shared<MessageLoopQt>());
  AddressSpaceImpl3 address_space_;
  std::shared_ptr<NodeService> node_service_override_ =
      MakeClientTestNodeService(address_space_);
  scada::MockServices services_;
  base::ScopedPathOverride private_dir_override_{client::DIR_PRIVATE};
  NiceMock<MockFunction<Awaitable<std::optional<DataServices>>(
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
  AnyExecutor captured_executor;

  auto capturing_configurator =
      [&, default_configurator = MakeUnitTestModules()](
          ClientApplicationModuleContext& ctx) {
        called = true;
        captured_profile = &ctx.profile_;
        captured_node_service = &ctx.node_service_;
        captured_task_manager = &ctx.task_manager_;
        captured_timed_data = &ctx.timed_data_service_;
        captured_controller_registry = &ctx.controller_registry_;
        captured_executor = ctx.executor_;
        default_configurator(ctx);
      };

  ClientApplication app{ClientApplicationContext{
      .io_context_ = io_context_,
      .executor_ = executor_,
      .login_handler_ = login_handler_.AsStdFunction(),
      .node_service_override_ = node_service_override_,
      .module_configurator_ = std::move(capturing_configurator),
  }};

  Wait(app.Start());

  EXPECT_TRUE(called);
  EXPECT_EQ(captured_profile, &app.profile());
  EXPECT_EQ(captured_timed_data, &app.timed_data_service());
  EXPECT_EQ(captured_controller_registry, &app.controller_registry());
  EXPECT_EQ(captured_executor, executor_);
  EXPECT_NE(captured_node_service, nullptr);
  EXPECT_NE(captured_task_manager, nullptr);

  Wait(app.Quit());
}

#endif
