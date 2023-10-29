#include "app/client_application.h"

#include "aui/wt/message_loop_wt.h"
#include "base/at_exit.h"
#include "base/bind_util.h"
#include "base/executor.h"
#include "base/task_runner_executor.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/login/wt/login_dialog.h"
#include "components/main/wt/main_window_wt.h"

#include <wt/WApplication.h>
#include <wt/WBootstrapTheme.h>
#include <wt/WBreak.h>
#include <wt/WContainerWidget.h>
#include <wt/WIOService.h>
#include <wt/WLineEdit.h>
#include <wt/WPushButton.h>
#include <wt/WServer.h>
#include <wt/WText.h>

class HelloApplication : public Wt::WApplication {
 public:
  HelloApplication(const Wt::WEnvironment& env,
                   boost::asio::io_context& io_context);
  ~HelloApplication();

 private:
  boost::asio::io_context& io_context_;
  scoped_refptr<MessageLoopWt> task_runner_{new MessageLoopWt{io_context_}};
  const std::shared_ptr<Executor> executor_ =
      std::make_shared<TaskRunnerExecutor>(task_runner_);
  // base::ThreadTaskRunnerHandle thread_task_runner_handle_{task_runner_};

  ClientApplication client_application_{ClientApplicationContext{
      io_context_,
      executor_,
      [this](MainWindowContext&& context) {
        return std::make_unique<MainWindowWt>(*root(), std::move(context));
      },
      [this](DataServicesContext&& services_context) {
        return ExecuteLoginDialog(executor_, *root(),
                                  std::move(services_context));
      },
      [] {},
  }};
};

HelloApplication::HelloApplication(const Wt::WEnvironment& env,
                                   boost::asio::io_context& io_context)
    : Wt::WApplication(env), io_context_{io_context} {
  setTheme(std::make_shared<Wt::WBootstrapTheme>());

  LOG(INFO) << "Connect";
  client_application_.Start();
}

HelloApplication::~HelloApplication() {
  LOG(INFO) << "Disconnect";
}

int main(int argc, char** argv) {
  base::AtExitManager at_exit;

  Wt::WIOService io_context;
  io_context.setThreadCount(1);

  int result = 0;
  try {
    Wt::WServer server{argc, argv};
    server.setIOService(io_context);
    server.addEntryPoint(
        Wt::EntryPointType::Application, [&](const Wt::WEnvironment& env) {
          return std::make_unique<HelloApplication>(env, io_context);
        });
    server.run();

  } catch (const std::exception& e) {
    LOG(ERROR) << "Error: " << e.what();
    result = -1;
  }

  return result;
}