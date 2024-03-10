#include "app/app_init.h"
#include "app/client_application.h"
#include "app/qt/installed_style.h"
#include "app/qt/installed_translation.h"
#include "aui/qt/message_loop_qt.h"
#include "base/task_runner_executor.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/win/gdiplus_initializer.h"
#include "components/login/login_dialog.h"
#include "main_window/qt/main_window_qt.h"
#include "project.h"
#include "services/atl_module.h"

#include <QApplication>
#include <QSettings>
#include <QTimer>
#include <boost/asio/io_context.hpp>

using namespace std::chrono_literals;

DummyAtlModule _Module;

int main(int argc, char* argv[]) {
  AppInit app_init;

  GdiplusInitializer gdiplus;

  QApplication qapp(argc, argv);
  qapp.setApplicationName("Telecontrol SCADA Client");
  qapp.setOrganizationName("Telecontrol");
  qapp.setOrganizationDomain("telecontrol.ru");
  qapp.setApplicationVersion(PROJECT_VERSION_DOTTED_STRING);
  qapp.setApplicationDisplayName(QObject::tr("Telecontrol SCADA Client"));
  qapp.setQuitOnLastWindowClosed(false);

  QSettings settings;
  InstalledTranslation installed_translation{settings};
  InstalledStyle installed_style{settings};

  // `QApplication` must be created.
  auto task_runner = base::MakeRefCounted<MessageLoopQt>();
  base::ThreadTaskRunnerHandle message_loop{task_runner};

  auto executor = std::make_shared<TaskRunnerExecutor>(task_runner);

  boost::asio::io_context io_context;
  ExecutorTimer io_context_poll{executor};
  io_context_poll.StartRepeating(10ms, [&io_context] { io_context.poll(); });

  ClientApplication app{ClientApplicationContext{
      .io_context_ = io_context,
      .executor_ = executor,
      .main_window_factory_ =
          [](MainWindowContext&& context) {
            return std::make_unique<MainWindowQt>(std::move(context));
          },
      .login_handler_ =
          [executor](DataServicesContext&& services_context) {
            return ExecuteLoginDialog(executor, std::move(services_context));
          }}};

  executor->PostTask([&app] { app.Run().then(&QApplication::quit); });

  return qapp.exec();
}