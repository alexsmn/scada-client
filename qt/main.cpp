#include "client_application.h"

#include "base/at_exit.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/win/gdiplus_initializer.h"
#include "components/main/qt/main_window_qt.h"
#include "qt/message_loop_qt.h"

#include <QApplication>
#include <QLibraryInfo>
#include <QSettings>
#include <QStyle>
#include <QTranslator>

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

  QSettings settings;

  // Save custom style.
  if (auto* style = QApplication::style())
    settings.setValue("Style", style->objectName());

  return result;
}
