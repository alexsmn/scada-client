#include "app/app_init.h"
#include "app/client_application.h"
#include "app/startup_exception.h"
#include "app/qt/installed_style.h"
#include "app/qt/installed_translation.h"
#include "aui/qt/message_loop_qt.h"
#include "base/boost_log.h"
#include "base/executor_timer.h"
#include "base/win/gdiplus_initializer.h"
#include "components/login/login_dialog.h"
#include "project.h"
#include "services/atl_module.h"

#include <Windows.h>
#include <QApplication>
#include <QSettings>
#include <QTimer>
#include <boost/asio/io_context.hpp>
#include <cstring>

using namespace std::chrono_literals;

DummyAtlModule _Module;

namespace {

void ShowStartupTrace(const wchar_t* message) {
  OutputDebugStringW(message);
  OutputDebugStringW(L"\n");
  MessageBoxW(nullptr, message, L"SCADA Client Startup", MB_OK | MB_ICONERROR);
}

void LogStartupException(std::exception_ptr exception) {
  if (auto description = DescribeStartupException(exception)) {
    BOOST_LOG_TRIVIAL(error) << "Client startup failed: " << *description;
  }
}

}  // namespace

int main(int argc, char* argv[]) {
  try {
    AppInit app_init;

    GdiplusInitializer gdiplus;

    QApplication qapp(argc, argv);

    QApplication::setApplicationName("Telecontrol SCADA Client");
    QApplication::setOrganizationName("Telecontrol");
    QApplication::setOrganizationDomain("telecontrol.ru");
    QApplication::setApplicationVersion(PROJECT_VERSION_DOTTED_STRING);
    QApplication::setApplicationDisplayName(
        QObject::tr("Telecontrol SCADA Client"));
    QApplication::setQuitOnLastWindowClosed(false);

    QSettings settings;
    InstalledTranslation installed_translation{settings};
    InstalledStyle installed_style{settings};

    // `QApplication` must be created.
    auto executor = std::make_shared<MessageLoopQt>();

    boost::asio::io_context io_context;
    ExecutorTimer io_context_poll{executor};
    io_context_poll.StartRepeating(10ms, [&io_context] { io_context.poll(); });

    ClientApplication app{ClientApplicationContext{
        .io_context_ = io_context,
        .executor_ = executor,
        .login_handler_ = [executor](DataServicesContext&& services_context) {
          return ExecuteLoginDialog(executor, std::move(services_context));
        }}};

    executor->PostTask([&app] {
      app.Start()
          .then([&app] { return app.Run(); })
          .except([](std::exception_ptr exception) {
            LogStartupException(exception);
          })
          .then(&QApplication::quit);
    });

    return QApplication::exec();
  } catch (const std::exception& e) {
    std::wstring message = L"main() exception: ";
    message += std::wstring{e.what(), e.what() + std::strlen(e.what())};
    ShowStartupTrace(message.c_str());
    return -1;
  } catch (...) {
    ShowStartupTrace(L"main() failed with an unknown exception");
    return -1;
  }
}
