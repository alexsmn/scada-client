#include "components/login/login_controller.h"

#include "aui/dialog_service_mock.h"
#include "base/memory_settings_store.h"
#include "base/test/awaitable_test.h"
#include "base/test/test_executor.h"
#include "scada/data_services_factory.h"
#include "scada/session_service_mock.h"
#include "scada/status_exception.h"
#include "scada/status_promise.h"

#include <gmock/gmock.h>
#include <transport/transport_factory.h>

#include <algorithm>
#include <memory>

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

REGISTER_DATA_SERVICES("Scada", u"Telecontrol", CreateScadaStubDataServices, "");
REGISTER_DATA_SERVICES("Vidicon", u"Vidicon", CreateVidiconStubDataServices, "");

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
    std::shared_ptr<Executor> executor,
    transport::TransportFactory& transport_factory) {
  return {.logger = {},
          .executor = std::move(executor),
          .transport_factory = transport_factory,
          .service_log_params = {}};
}

std::shared_ptr<TestLoginController> CreateController(
    std::shared_ptr<Executor> executor,
    DialogService& dialog_service,
    std::shared_ptr<SettingsStore> settings_store,
    transport::TransportFactory& transport_factory) {
  auto controller = std::make_shared<TestLoginController>(
      executor, MakeServicesContext(executor, transport_factory), dialog_service,
      std::move(settings_store));
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

  auto executor = std::make_shared<TestExecutor>(true);
  StrictMock<MockDialogService> dialog_service;
  NullTransportFactory transport_factory;

  DataServicesContext services_context{
      .logger = {},
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

  auto executor = std::make_shared<TestExecutor>(true);
  StrictMock<MockDialogService> dialog_service;
  NullTransportFactory transport_factory;

  DataServicesContext services_context{
      .logger = {},
      .executor = executor,
      .transport_factory = transport_factory,
      .service_log_params = {}};

  TestLoginController controller{executor, std::move(services_context),
                                 dialog_service, settings_store};

  const int scada_index = FindServerTypeIndex("Scada");
  EXPECT_EQ(controller.server_type_index(), scada_index);
  EXPECT_EQ(controller.server_host, "scada-host");
}

TEST(LoginControllerTest, LoginCompletesAfterSessionConnect) {
  auto settings_store = std::make_shared<MemorySettingsStore>();
  auto executor = std::make_shared<TestExecutor>();
  StrictMock<MockDialogService> dialog_service;
  StrictMock<scada::MockSessionService> session_service;
  ScopedScadaSessionService scoped_session_service{session_service};
  NullTransportFactory transport_factory;
  promise<void> connect;
  bool completed = false;

  EXPECT_CALL(session_service, Connect(_))
      .WillOnce(DoAll(WithArg<0>([](const scada::SessionConnectParams& params) {
                        EXPECT_EQ(params.host, "scada-host");
                        EXPECT_EQ(params.user_name, u"ivan");
                        EXPECT_EQ(params.password, u"secret");
                        EXPECT_FALSE(params.allow_remote_logoff);
                      }),
                      Return(connect)));

  auto controller = CreateController(executor, dialog_service, settings_store,
                                     transport_factory);
  controller->completion_handler = [&](DataServices services) {
    completed = true;
    EXPECT_EQ(services.session_service_.get(), &session_service);
  };

  controller->Login();
  Drain(executor);
  EXPECT_FALSE(completed);

  connect.resolve();
  Drain(executor);

  EXPECT_TRUE(completed);
  EXPECT_EQ(settings_store->GetString16("User"),
            std::optional<std::u16string>{u"ivan"});
}

TEST(LoginControllerTest, FailedLoginReportsErrorAfterMessageBox) {
  auto settings_store = std::make_shared<MemorySettingsStore>();
  auto executor = std::make_shared<TestExecutor>();
  StrictMock<MockDialogService> dialog_service;
  StrictMock<scada::MockSessionService> session_service;
  ScopedScadaSessionService scoped_session_service{session_service};
  NullTransportFactory transport_factory;
  promise<void> connect;
  promise<MessageBoxResult> error_message;
  bool error_reported = false;

  EXPECT_CALL(session_service, Connect(_)).WillOnce(Return(connect));
  EXPECT_CALL(dialog_service,
              RunMessageBox(/*message=*/_, /*title=*/_,
                            MessageBoxMode::Error))
      .WillOnce(Return(error_message));

  auto controller = CreateController(executor, dialog_service, settings_store,
                                     transport_factory);
  controller->error_handler = [&] { error_reported = true; };

  controller->Login();
  Drain(executor);
  connect.reject(scada::status_exception{scada::StatusCode::Bad});
  Drain(executor);

  EXPECT_FALSE(error_reported);

  error_message.resolve(MessageBoxResult::Ok);
  Drain(executor);

  EXPECT_TRUE(error_reported);
}

TEST(LoginControllerTest, ForceLogoffPromptRetriesConnectWhenAccepted) {
  auto settings_store = std::make_shared<MemorySettingsStore>();
  auto executor = std::make_shared<TestExecutor>();
  StrictMock<MockDialogService> dialog_service;
  StrictMock<scada::MockSessionService> session_service;
  ScopedScadaSessionService scoped_session_service{session_service};
  NullTransportFactory transport_factory;
  promise<void> first_connect;
  promise<void> second_connect;
  promise<MessageBoxResult> force_logoff_message;
  bool completed = false;

  EXPECT_CALL(session_service, Connect(_))
      .WillOnce(DoAll(WithArg<0>([](const scada::SessionConnectParams& params) {
                        EXPECT_FALSE(params.allow_remote_logoff);
                      }),
                      Return(first_connect)))
      .WillOnce(DoAll(WithArg<0>([](const scada::SessionConnectParams& params) {
                        EXPECT_TRUE(params.allow_remote_logoff);
                      }),
                      Return(second_connect)));
  EXPECT_CALL(dialog_service,
              RunMessageBox(/*message=*/_, /*title=*/_,
                            MessageBoxMode::QuestionYesNo))
      .WillOnce(Return(force_logoff_message));

  auto controller = CreateController(executor, dialog_service, settings_store,
                                     transport_factory);
  controller->completion_handler = [&](DataServices) { completed = true; };

  controller->Login();
  Drain(executor);
  first_connect.reject(scada::status_exception{
      scada::StatusCode::Bad_UserIsAlreadyLoggedOn});
  Drain(executor);

  force_logoff_message.resolve(MessageBoxResult::Yes);
  Drain(executor);
  EXPECT_FALSE(completed);

  second_connect.resolve();
  Drain(executor);

  EXPECT_TRUE(completed);
}

TEST(LoginControllerTest, DestroyedControllerDropsPendingConnectCompletion) {
  auto settings_store = std::make_shared<MemorySettingsStore>();
  auto executor = std::make_shared<TestExecutor>();
  StrictMock<MockDialogService> dialog_service;
  StrictMock<scada::MockSessionService> session_service;
  ScopedScadaSessionService scoped_session_service{session_service};
  NullTransportFactory transport_factory;
  promise<void> connect;
  bool completed = false;

  EXPECT_CALL(session_service, Connect(_)).WillOnce(Return(connect));

  auto controller = CreateController(executor, dialog_service, settings_store,
                                     transport_factory);
  controller->completion_handler = [&](DataServices) { completed = true; };

  controller->Login();
  Drain(executor);
  controller.reset();

  connect.resolve();
  Drain(executor);

  EXPECT_FALSE(completed);
}

}  // namespace
