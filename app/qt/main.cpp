#include "app/client_application.h"
#include "base/at_exit.h"
#include "base/qt/message_loop_qt.h"
#include "base/task_runner_executor.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/win/gdiplus_initializer.h"
#include "components/login/login_dialog.h"
#include "components/main/qt/main_window_qt.h"
#include "project.h"
#include "services/atl_module.h"

#include <QApplication>
#include <QLibraryInfo>
#include <QSettings>
#include <QStyle>
#include <QTimer>
#include <QTranslator>
#include <boost/asio/io_context.hpp>

using namespace std::chrono_literals;

namespace {
const char kDefaultStyle[] = "Fusion";
}

DummyAtlModule _Module;

int main(int argc, char* argv[]) {
  base::AtExitManager at_exit;

  GdiplusInitializer gdiplus;

  QApplication qapp(argc, argv);
  qapp.setApplicationName("Telecontrol SCADA Client");
  qapp.setOrganizationName("Telecontrol");
  qapp.setOrganizationDomain("telecontrol.ru");
  qapp.setApplicationVersion(PROJECT_VERSION_DOTTED_STRING);

  QTranslator qt_translator;
  QTranslator app_translator;

  {
    QSettings settings;

    // Set custom style.
    auto style = settings.value("Style").toString();
    if (style.isEmpty())
      style = kDefaultStyle;
    QApplication::setStyle(style);

    // Load translations.
    auto system_name = settings.value("SystemName").toString();
    if (system_name.isEmpty())
      system_name = QLocale::system().name();

    const auto local_translation_dir =
        QApplication::applicationDirPath() + "/translations";
    const auto global_translation_dir =
        QLibraryInfo::location(QLibraryInfo::TranslationsPath);

    const auto qt_translation_name = "qt_" + system_name;
    if (qt_translator.load(qt_translation_name, local_translation_dir) ||
        qt_translator.load(qt_translation_name, global_translation_dir))
      QApplication::installTranslator(&qt_translator);

    const auto client_translation_name = "client_" + system_name;
    if (app_translator.load(client_translation_name, local_translation_dir))
      QApplication::installTranslator(&app_translator);
  }

  qapp.setApplicationDisplayName(QObject::tr("Telecontrol SCADA Client"));

  qapp.setQuitOnLastWindowClosed(false);

  // QApplication must be created.
  auto task_runner = base::MakeRefCounted<MessageLoopQt>();
  base::ThreadTaskRunnerHandle message_loop{task_runner};

  auto executor = std::make_shared<TaskRunnerExecutor>(task_runner);

  boost::asio::io_context io_context;
  ExecutorTimer io_context_poll{executor};
  io_context_poll.StartRepeating(10ms, [&io_context] { io_context.poll(); });

  ClientApplication app{ClientApplicationContext{
      io_context, executor,
      [](MainWindowContext&& context) {
        return std::make_unique<MainWindowQt>(std::move(context));
      },
      [executor](DataServicesContext&& services_context) {
        return ExecuteLoginDialog(executor, std::move(services_context));
      },
      [&qapp] { qapp.quit(); }}};

  Dispatch(*executor, [&app] { app.Start(); });

  int result = qapp.exec();

  QSettings settings;

  // Save custom style.
  if (auto* style = QApplication::style())
    settings.setValue("Style", style->objectName());

  return result;
}