#include "modules/login/login_controller.h"

#include "aui/dialog_service_mock.h"
#include "aui/translation.h"
#include "base/callback_awaitable.h"
#include "base/memory_settings_store.h"
#include "base/test/awaitable_test.h"
#include "base/test/test_executor.h"
#include "scada/co_result.h"
#include "scada/data_services_factory.h"
#include "scada/session_service_mock.h"

#include <gmock/gmock.h>
#include <transport/transport_factory.h>

#include <algorithm>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

using namespace testing;

namespace {

class NullTransportFactory final : public transport::TransportFactory {
 public:
  transport::expected<transport::any_transport> CreateTransport(
      const transport::TransportString&,
      const transport::executor&,
      const transport::log_source&) override {
    return transport::ERR_NOT_IMPLEMENTED;
  }
};

class DeferredStatus {
 public:
  scada::CoStatus Wait(AnyExecutor executor) {
    auto [status] = co_await CallbackToAwaitable<scada::Status>(
        std::move(executor),
        [this](auto callback) { callback_ = std::move(callback); });
    co_return status;
  }

  void Resolve(scada::Status status = scada::StatusCode::Good) {
    callback_(std::move(status));
  }

 private:
  std::function<void(scada::Status)> callback_;
};

template <class T>
class DeferredValue {
 public:
  Awaitable<T> Wait(AnyExecutor executor) {
    auto [error, value] = co_await CallbackToAwaitable<std::exception_ptr, T>(
        std::move(executor),
        [this](auto callback) { callback_ = std::move(callback); });
    if (error) {
      std::rethrow_exception(error);
    }
    co_return std::move(value);
  }

  void Resolve(T value) { callback_(nullptr, std::move(value)); }

 private:
  std::function<void(std::exception_ptr, T)> callback_;
};

#if defined(_MSC_VER)
#define TEST_NOINLINE __declspec(noinline)
#else
#define TEST_NOINLINE __attribute__((noinline))
#endif

// Overwrites the stack region below the current frame so that a dangling
// reference into an already-popped frame reads garbage instead of
// coincidentally intact bytes. Turns stack-lifetime bugs (e.g. a coroutine
// resuming through a dead temporary closure) into deterministic failures.
TEST_NOINLINE void ScribbleStack() {
  std::uint8_t garbage[16384];
  volatile std::uint8_t* p = garbage;
  for (size_t i = 0; i < sizeof(garbage); ++i)
    p[i] = 0xAB;
}

// Runs ready tasks batch by batch, scribbling the stack between batches, so
// no queued continuation can rely on dead stack memory surviving until it
// resumes.
void DrainWithStackScribble(TestExecutor& executor) {
  while (executor.HasReadyTasks()) {
    executor.Poll();
    ScribbleStack();
  }
}

scada::SessionService* scada_session_service = nullptr;

bool CreateScadaStubDataServices(const DataServicesContext&,
                                 DataServices& services) {
  if (scada_session_service) {
    services.session_service_ = std::shared_ptr<scada::SessionService>(
        scada_session_service, [](scada::SessionService*) {});
  }
  return true;
}

bool CreateVidiconStubDataServices(const DataServicesContext&, DataServices&) {
  return true;
}

REGISTER_DATA_SERVICES("Scada",
                       u"Telecontrol",
                       CreateScadaStubDataServices,
                       "");
REGISTER_DATA_SERVICES("Vidicon",
                       u"Vidicon",
                       CreateVidiconStubDataServices,
                       "");

class TestLoginController : public LoginController {
 public:
  using LoginController::LoginController;

  void CompleteLoginForTest(std::string server_type) {
    server_type_ = std::move(server_type);
    OnLoginCompleted();
  }
};

int FindServerTypeIndex(std::string_view name) {
  const auto& services = GetDataServicesInfoList();
  const auto it = std::ranges::find_if(
      services, [name](const auto& info) { return info.name == name; });
  EXPECT_NE(it, services.end());
  return static_cast<int>(std::distance(services.begin(), it));
}

DataServicesContext MakeServicesContext(
    AnyExecutor executor,
    transport::TransportFactory& transport_factory) {
  return {.logger = {},
          .executor = std::move(executor),
          .transport_factory = transport_factory,
          .service_log_params = {}};
}

std::shared_ptr<TestLoginController> CreateController(
    AnyExecutor executor,
    DialogService& dialog_service,
    std::shared_ptr<SettingsStore> settings_store,
    transport::TransportFactory& transport_factory) {
  auto controller = std::make_shared<TestLoginController>(
      executor, MakeServicesContext(executor, transport_factory),
      dialog_service, std::move(settings_store));
  controller->SetServerTypeIndex(FindServerTypeIndex("Scada"));
  controller->server_host = "scada-host";
  controller->user_name = u"ivan";
  controller->password = u"secret";
  controller->auto_login = false;
  return controller;
}

class ScopedScadaSessionService {
 public:
  explicit ScopedScadaSessionService(scada::SessionService& session_service)
      : previous_{scada_session_service} {
    scada_session_service = &session_service;
  }

  ~ScopedScadaSessionService() { scada_session_service = previous_; }

 private:
  scada::SessionService* previous_ = nullptr;
};

TEST(LoginControllerTest, PersistsEnglishServerTypeAndHostKey) {
  auto settings_store = std::make_shared<MemorySettingsStore>();
  settings_store->SetString("ServerType", "Scada");
  settings_store->SetString("Host:Scada", "localhost");

  TestExecutor executor{true};
  StrictMock<MockDialogService> dialog_service;
  NullTransportFactory transport_factory;

  DataServicesContext services_context{.logger = {},
                                       .executor = executor,
                                       .transport_factory = transport_factory,
                                       .service_log_params = {}};

  TestLoginController controller{executor, std::move(services_context),
                                 dialog_service, settings_store};
  controller.completion_handler = [](DataServices) {};

  const int scada_index = FindServerTypeIndex("Scada");
  controller.SetServerTypeIndex(scada_index);
  controller.server_type_list[scada_index] = u"Телеконтроль";
  controller.server_host = "scada-host";
  controller.user_name = u"ivan";
  controller.user_list.clear();
  controller.auto_login = false;

  controller.CompleteLoginForTest("Scada");

  EXPECT_EQ(settings_store->GetString("ServerType"),
            std::optional<std::string>{"Scada"});
  EXPECT_EQ(settings_store->GetString("Host:Scada"),
            std::optional<std::string>{"scada-host"});
  EXPECT_NE(settings_store->GetString("ServerType"),
            std::optional<std::string>{"Telecontrol"});
}

TEST(LoginControllerTest, ReadsStoredEnglishServerTypeIntoSelectedIndex) {
  auto settings_store = std::make_shared<MemorySettingsStore>();
  settings_store->SetString("ServerType", "Scada");
  settings_store->SetString("Host:Scada", "scada-host");
  settings_store->SetString("Host:Vidicon", "vidicon-host");

  TestExecutor executor{true};
  StrictMock<MockDialogService> dialog_service;
  NullTransportFactory transport_factory;

  DataServicesContext services_context{.logger = {},
                                       .executor = executor,
                                       .transport_factory = transport_factory,
                                       .service_log_params = {}};

  TestLoginController controller{executor, std::move(services_context),
                                 dialog_service, settings_store};

  const int scada_index = FindServerTypeIndex("Scada");
  EXPECT_EQ(controller.server_type_index(), scada_index);
  EXPECT_EQ(controller.server_host, "scada-host");
}

// The session carries the language the client is displayed in, so
// server-supplied text — node display names above all — comes back in it
// rather than in whatever language the configuration was authored in. OPC UA
// Part 4 §5.4 Locale Negotiation,
// https://reference.opcfoundation.org/Core/Part4/v105/docs/5.4
//
// This is the one link in that chain with nothing else behind it: the wire and
// the server are covered by SessionProxyTest and the framework suites, and
// `UiLocaleName()` by its own tests, but the controller putting one into the
// other is a plain pass-through that only a test can hold in place.
TEST(LoginControllerTest, ConnectCarriesTheUiLanguageAsTheSessionLocale) {
  auto settings_store = std::make_shared<MemorySettingsStore>();
  TestExecutor executor;
  StrictMock<MockDialogService> dialog_service;
  StrictMock<scada::MockSessionService> session_service;
  ScopedScadaSessionService scoped_session_service{session_service};
  NullTransportFactory transport_factory;
  DeferredStatus connect;

  EXPECT_CALL(session_service, ConnectStatus(_))
      .WillOnce([executor, &connect](
                    scada::SessionConnectParams params) -> scada::CoStatus {
        // Exactly the UI language, most preferred and alone: this client shows
        // one language at a time, so it asks for one.
        EXPECT_EQ(std::vector<std::string>{UiLocaleName()}, params.locale_ids);
        co_return co_await connect.Wait(executor);
      });

  auto controller = CreateController(executor, dialog_service, settings_store,
                                     transport_factory);
  controller->completion_handler = [](DataServices) {};
  controller->Login();
  Drain(executor);
  connect.Resolve();
  Drain(executor);
}

TEST(LoginControllerTest, LoginCompletesAfterSessionConnect) {
  auto settings_store = std::make_shared<MemorySettingsStore>();
  TestExecutor executor;
  StrictMock<MockDialogService> dialog_service;
  StrictMock<scada::MockSessionService> session_service;
  ScopedScadaSessionService scoped_session_service{session_service};
  NullTransportFactory transport_factory;
  DeferredStatus connect;
  bool completed = false;

  EXPECT_CALL(session_service, ConnectStatus(_))
      .WillOnce([executor, &connect](
                    scada::SessionConnectParams params) -> scada::CoStatus {
        EXPECT_EQ(params.host, "scada-host");
        EXPECT_EQ(params.user_name, u"ivan");
        EXPECT_EQ(params.password, u"secret");
        EXPECT_FALSE(params.allow_remote_logoff);
        co_return co_await connect.Wait(executor);
      });

  auto controller = CreateController(executor, dialog_service, settings_store,
                                     transport_factory);
  controller->completion_handler = [&](DataServices services) {
    completed = true;
    EXPECT_EQ(services.session_service_.get(), &session_service);
  };

  controller->Login();
  Drain(executor);
  EXPECT_FALSE(completed);

  connect.Resolve();
  Drain(executor);

  EXPECT_TRUE(completed);
  EXPECT_EQ(settings_store->GetString16("User"),
            std::optional<std::u16string>{u"ivan"});
}

// Regression test: the auto-login info prompt used to be built by an
// immediately-invoked capturing lambda coroutine whose temporary closure died
// before the awaitable was awaited inside CompleteLoginAsync, so resuming it
// read dead stack memory. The prompt awaitable must survive until the spawned
// completion coroutine awaits it, and completion must wait for the prompt.
TEST(LoginControllerTest, AutoLoginShowsInfoMessageBeforeCompletion) {
  auto settings_store = std::make_shared<MemorySettingsStore>();
  TestExecutor executor;
  StrictMock<MockDialogService> dialog_service;
  StrictMock<scada::MockSessionService> session_service;
  ScopedScadaSessionService scoped_session_service{session_service};
  NullTransportFactory transport_factory;
  DeferredStatus connect;
  DeferredValue<MessageBoxResult> auto_login_message;
  bool completed = false;

  EXPECT_CALL(session_service, ConnectStatus(_))
      .WillOnce(
          [executor, &connect](scada::SessionConnectParams) -> scada::CoStatus {
            co_return co_await connect.Wait(executor);
          });
  EXPECT_CALL(dialog_service,
              RunMessageBox(/*message=*/_, /*title=*/_, MessageBoxMode::Info))
      .WillOnce([executor, &auto_login_message](
                    std::u16string_view, std::u16string_view,
                    MessageBoxMode) -> Awaitable<MessageBoxResult> {
        co_return co_await auto_login_message.Wait(executor);
      });

  auto controller = CreateController(executor, dialog_service, settings_store,
                                     transport_factory);
  // The constructor sets login_message_ = true because the stored AutoLogin
  // flag is false; enabling auto_login here selects the prompt path.
  controller->auto_login = true;
  controller->completion_handler = [&](DataServices) { completed = true; };

  controller->Login();
  Drain(executor);
  connect.Resolve();
  // Scribble between task batches: the prompt awaitable is built in
  // OnLoginCompleted but first awaited in a later batch, so it must not
  // reference anything on OnLoginCompleted's stack.
  DrainWithStackScribble(executor);

  EXPECT_FALSE(completed);

  auto_login_message.Resolve(MessageBoxResult::Ok);
  Drain(executor);

  EXPECT_TRUE(completed);
}

TEST(LoginControllerTest, FailedLoginReportsErrorAfterMessageBox) {
  auto settings_store = std::make_shared<MemorySettingsStore>();
  TestExecutor executor;
  StrictMock<MockDialogService> dialog_service;
  StrictMock<scada::MockSessionService> session_service;
  ScopedScadaSessionService scoped_session_service{session_service};
  NullTransportFactory transport_factory;
  DeferredStatus connect;
  DeferredValue<MessageBoxResult> error_message;
  bool error_reported = false;

  EXPECT_CALL(session_service, ConnectStatus(_))
      .WillOnce(
          [executor, &connect](scada::SessionConnectParams) -> scada::CoStatus {
            co_return co_await connect.Wait(executor);
          });
  EXPECT_CALL(dialog_service,
              RunMessageBox(/*message=*/_, /*title=*/_, MessageBoxMode::Error))
      .WillOnce([executor, &error_message](
                    std::u16string_view, std::u16string_view,
                    MessageBoxMode) -> Awaitable<MessageBoxResult> {
        co_return co_await error_message.Wait(executor);
      });

  auto controller = CreateController(executor, dialog_service, settings_store,
                                     transport_factory);
  controller->error_handler = [&] { error_reported = true; };

  controller->Login();
  Drain(executor);
  connect.Resolve(scada::StatusCode::Bad);
  Drain(executor);

  EXPECT_FALSE(error_reported);

  error_message.Resolve(MessageBoxResult::Ok);
  Drain(executor);

  EXPECT_TRUE(error_reported);
}

TEST(LoginControllerTest, ForceLogoffPromptRetriesConnectWhenAccepted) {
  auto settings_store = std::make_shared<MemorySettingsStore>();
  TestExecutor executor;
  StrictMock<MockDialogService> dialog_service;
  StrictMock<scada::MockSessionService> session_service;
  ScopedScadaSessionService scoped_session_service{session_service};
  NullTransportFactory transport_factory;
  DeferredStatus first_connect;
  DeferredStatus second_connect;
  DeferredValue<MessageBoxResult> force_logoff_message;
  bool completed = false;

  EXPECT_CALL(session_service, ConnectStatus(_))
      .WillOnce([executor, &first_connect](
                    scada::SessionConnectParams params) -> scada::CoStatus {
        EXPECT_FALSE(params.allow_remote_logoff);
        co_return co_await first_connect.Wait(executor);
      })
      .WillOnce([executor, &second_connect](
                    scada::SessionConnectParams params) -> scada::CoStatus {
        EXPECT_TRUE(params.allow_remote_logoff);
        co_return co_await second_connect.Wait(executor);
      });
  EXPECT_CALL(dialog_service, RunMessageBox(/*message=*/_, /*title=*/_,
                                            MessageBoxMode::QuestionYesNo))
      .WillOnce([executor, &force_logoff_message](
                    std::u16string_view, std::u16string_view,
                    MessageBoxMode) -> Awaitable<MessageBoxResult> {
        co_return co_await force_logoff_message.Wait(executor);
      });

  auto controller = CreateController(executor, dialog_service, settings_store,
                                     transport_factory);
  controller->completion_handler = [&](DataServices) { completed = true; };

  controller->Login();
  Drain(executor);
  first_connect.Resolve(scada::StatusCode::Bad_UserIsAlreadyLoggedOn);
  Drain(executor);

  force_logoff_message.Resolve(MessageBoxResult::Yes);
  Drain(executor);
  EXPECT_FALSE(completed);

  second_connect.Resolve();
  Drain(executor);

  EXPECT_TRUE(completed);
}

TEST(LoginControllerTest, DestroyedControllerDropsPendingConnectCompletion) {
  auto settings_store = std::make_shared<MemorySettingsStore>();
  TestExecutor executor;
  StrictMock<MockDialogService> dialog_service;
  StrictMock<scada::MockSessionService> session_service;
  ScopedScadaSessionService scoped_session_service{session_service};
  NullTransportFactory transport_factory;
  DeferredStatus connect;
  bool completed = false;

  EXPECT_CALL(session_service, ConnectStatus(_))
      .WillOnce(
          [executor, &connect](scada::SessionConnectParams) -> scada::CoStatus {
            co_return co_await connect.Wait(executor);
          });

  auto controller = CreateController(executor, dialog_service, settings_store,
                                     transport_factory);
  controller->completion_handler = [&](DataServices) { completed = true; };

  controller->Login();
  Drain(executor);
  controller.reset();

  connect.Resolve();
  Drain(executor);

  EXPECT_FALSE(completed);
}

}  // namespace
