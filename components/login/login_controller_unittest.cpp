#include "components/login/login_controller.h"

#include "aui/dialog_service_mock.h"
#include "base/memory_settings_store.h"
#include "base/test/test_executor.h"
#include "scada/data_services_factory.h"

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

bool CreateScadaStubDataServices(const DataServicesContext&, DataServices&) {
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

}  // namespace
