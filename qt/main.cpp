#include "base/at_exit.h"
#include "base/task_runner_executor.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/win/gdiplus_initializer.h"
#include "client_application.h"
#include "components/login/login_dialog.h"
#include "components/main/qt/main_window_qt.h"
#include "project.h"
#include "qt/message_loop_qt.h"

#include <QApplication>
#include <QLibraryInfo>
#include <QSettings>
#include <QStyle>
#include <QTimer>
#include <QTranslator>
#include <boost/asio/io_context.hpp>

namespace {
const char kDefaultStyle[] = "Fusion";
}

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

  boost::asio::io_context io_context;

  // QApplication must be created.
  auto task_runner = base::MakeRefCounted<MessageLoopQt>();
  base::ThreadTaskRunnerHandle message_loop{task_runner};

  auto executor = std::make_shared<TaskRunnerExecutor>(task_runner);

  QTimer timer;
  timer.setInterval(10);
  QObject::connect(&timer, &QTimer::timeout, [&io_context, task_runner] {
    task_runner->Run();
    io_context.poll();
  });
  timer.start();

  int result = 0;

  try {
    ClientApplication app{ClientApplicationContext{
        io_context, executor,
        [](MainWindowContext&& context) {
          return std::make_unique<MainWindowQt>(std::move(context));
        },
        [executor](DataServicesContext&& services_context) {
          return ExecuteLoginDialog(executor, std::move(services_context));
        },
        [&qapp] { qapp.quit(); }}};

    executor->PostTask([&app] { app.Start(); });

    result = qapp.exec();

  } catch (const std::exception&) {
    result = -1;
  }

  QSettings settings;

  // Save custom style.
  if (auto* style = QApplication::style())
    settings.setValue("Style", style->objectName());

  return result;
}