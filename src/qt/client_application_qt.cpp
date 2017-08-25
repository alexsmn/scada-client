#include "client/qt/client_application_qt.h"

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "client/client_paths.h"
#include "client/components/main/qt/main_window_qt.h"
#include "client/components/login/qt/login_dialog.h"

ClientApplicationQt* g_application_qt = nullptr;

ClientApplicationQt::ClientApplicationQt(int argc, char** argv)
    : QApplication(argc, argv),
      ClientApplication(argc, argv) {
}

bool ClientApplicationQt::Init() {
  if (!ClientApplication::Init())
    return false;

  // Load style-sheet.
  {
    base::FilePath path;
    PathService::Get(client::DIR_PUBLIC, &path);
    path = path.Append(FILE_PATH_LITERAL("style-sheet"));
    std::string style_sheet;
    if (base::ReadFileToString(path, &style_sheet))
      setStyleSheet(QString::fromStdString(style_sheet));
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

bool ClientApplicationQt::ShowLoginDialog() {
  LoginDialog login_dialog(session_service());
  return login_dialog.exec() == QDialog::Accepted;
}
