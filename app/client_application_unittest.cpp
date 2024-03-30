#include "client_application.h"

#include "aui/test/app_environment.h"
#include "base/client_paths.h"
#include "base/test/scoped_path_override.h"
#include "base/test/test_executor.h"
#include "profile/profile.h"
#include "scada/services_mock.h"

#include <boost/asio/io_context.hpp>
#include <gmock/gmock.h>

using namespace ::testing;

namespace {

Page MakeAllWindowsPage(ControllerRegistry& controller_registry) {
  Page page;

  return page;
}

}  // namespace

class ClientApplicationTest : public Test {
 public:
  ClientApplicationTest();

 protected:
  boost::asio::io_context io_context_;
  std::shared_ptr<Executor> executor_ = std::make_shared<TestExecutor>();
  AppEnvironment app_env_;
  scada::MockServices services_;

  base::ScopedPathOverride private_dir_override_{client::DIR_PRIVATE};

  StrictMock<MockFunction<promise<std::optional<DataServices>>(
      DataServicesContext&& services_context)>>
      login_handler_;

  ClientApplication app_{ClientApplicationContext{
      .io_context_ = io_context_,
      .executor_ = executor_,
      .login_handler_ = login_handler_.AsStdFunction()}};
};

ClientApplicationTest::ClientApplicationTest() {
  ON_CALL(login_handler_, Call(/*services_context=*/_))
      .WillByDefault(Return(make_resolved_promise(std::optional{
          DataServices::FromUnownedServices(services_.services())})));
}

TEST_F(ClientApplicationTest, LoginFailed) {
  EXPECT_CALL(login_handler_, Call(/*services_context=*/_))
      .WillOnce(Return(make_resolved_promise(std::optional<DataServices>{})));

  EXPECT_THROW(app_.Start().get(), std::runtime_error);
}

// Ensure that the initial page is created and all windows are defined.
TEST_F(ClientApplicationTest, RunWithNewProfile) {
  EXPECT_CALL(login_handler_, Call(/*services_context=*/_));

  app_.Start().get();
  app_.Quit().get();
}

// Ensure that the initial page is created and all windows are defined.
TEST_F(ClientApplicationTest, OpenAllWindows) {
  EXPECT_CALL(login_handler_, Call(/*services_context=*/_));

  {
    Profile profile;
    profile.AddPage(MakeAllWindowsPage(app_.controller_registry()));
    profile.Save();
  }

  app_.Start().get();
  app_.Quit().get();
}
