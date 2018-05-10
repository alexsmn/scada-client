#include "client_application.h"

#include "base/at_exit.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/win/gdiplus_initializer.h"
#include "client_paths.h"
#include "components/main/qt/main_window_qt.h"
#include "qt/message_loop_qt.h"

#include <QApplication>
#include <QLibraryInfo>
#include <QSettings>
#include <QStyle>
#include <QTranslator>

namespace {
const char kRegistryKey[] =
    "HKEY_CURRENT_USER\\Software\\Telecontrol\\Workplace";
}

int main(int argc, char* argv[]) {
  GdiplusInitializer gdiplus;

  base::AtExitManager at_exit;

  QApplication qapp(argc, argv);

  // Load translations.
  {
    QTranslator qt_translator;
    qt_translator.load("qt_" + QLocale::system().name(),
                       QLibraryInfo::location(QLibraryInfo::TranslationsPath));
    QApplication::installTranslator(&qt_translator);

    QTranslator app_translator;
    app_translator.load("client_" + QLocale::system().name());
    QApplication::installTranslator(&app_translator);
  }

  // Set custom style.
  {
    // TODO: Const
    QSettings settings{kRegistryKey, QSettings::NativeFormat};
    // TODO: Const
    auto style = settings.value("Style").toString();
    if (!style.isEmpty())
      QApplication::setStyle(style);
  }

  // QApplication must be created.
  base::ThreadTaskRunnerHandle message_loop{new MessageLoopQt};

  int result = 0;

  try {
    ClientApplication app{ClientApplicationContext{
        [](MainWindowContext&& context) {
          return std::make_unique<MainWindowQt>(std::move(context));
        },
        [&qapp] { qapp.quit(); }}};

    if (!app.Login())
      throw std::runtime_error("Login failed");

    app.Start();
    result = qapp.exec();

  } catch (const std::exception&) {
    result = -1;
  }

  // Save custom style.
  if (auto* style = QApplication::style()) {
    // TODO: Const
    QSettings settings{kRegistryKey, QSettings::NativeFormat};
    // TODO: Const
    settings.setValue("Style", style->objectName());
  }

  return result;
}
