#include "qt/client_application_qt.h"

#include <QApplication>
#include <QLibraryInfo>
#include <QSettings>
#include <QTranslator>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "client_paths.h"
#include "components/main/qt/main_window_qt.h"
#include "components/login/qt/login_dialog.h"

ClientApplicationQt* g_application_qt = nullptr;

ClientApplicationQt::ClientApplicationQt(int argc, char** argv)
    : ClientApplication(argc, argv) {
}

ClientApplicationQt::~ClientApplicationQt() {
  // Save custom style.
  if (auto* style = QApplication::style()) {
    // TODO: Const
    QSettings settings("HKEY_CURRENT_USER\\Software\\Telecontrol\\Workplace", QSettings::NativeFormat);
    // TODO: Const
    settings.setValue("Style", style->objectName());
  }
}

bool ClientApplicationQt::Init() {
  if (!ClientApplication::Init())
    return false;

  // Load translations.
  {
    QTranslator qt_translator;
    qt_translator.load("qt_" + QLocale::system().name(), QLibraryInfo::location(QLibraryInfo::TranslationsPath));
    QApplication::installTranslator(&qt_translator);

    QTranslator app_translator;
    app_translator.load("client_" + QLocale::system().name());
    QApplication::installTranslator(&app_translator);
  }

  // Set custom style.
  {
    // TODO: Const
    QSettings settings("HKEY_CURRENT_USER\\Software\\Telecontrol\\Workplace", QSettings::NativeFormat);
    // TODO: Const
    auto style = settings.value("Style").toString();
    if (!style.isEmpty())
      QApplication::setStyle(style);
  }

  return true;
}

int ClientApplicationQt::Run(int show) {
  return QApplication::exec();
}

void ClientApplicationQt::Quit() {
  QApplication::quit();
}

std::unique_ptr<MainWindow> ClientApplicationQt::CreateMainWindow(MainWindowContext&& context) {
  return std::make_unique<MainWindowQt>(*this, std::move(context));
}

bool ClientApplicationQt::ShowLoginDialogImpl(const DataServicesContext& context, DataServices& services) {
  LoginDialog login_dialog(context);
  if (login_dialog.exec() != QDialog::Accepted)
    return false;
  services = login_dialog.services();
  return true;
}
