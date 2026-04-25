#include "app/app_init.h"
#include "app/client_application.h"
#include "app/startup_exception.h"
#include "app/qt/e2e_test_support.h"
#include "app/qt/installed_style.h"
#include "app/qt/installed_translation.h"
#include "aui/qt/message_loop_qt.h"
#include "base/e2e_test_hooks.h"
#include "base/boost_log.h"
#include "base/executor_timer.h"
#include "base/win/gdiplus_initializer.h"
#include "components/login/login_dialog.h"
#include "project.h"
#include "scada/status_exception.h"
#include "services/atl_module.h"

#include <Windows.h>
#include <QApplication>
#include <QSettings>
#include <QTimer>
#include <boost/asio/io_context.hpp>
#include <cstdlib>
#include <exception>
#include <memory>
#include <string>
#include <utility>

using namespace std::chrono_literals;

DummyAtlModule _Module;

namespace {

void LogQtEventException(std::exception_ptr exception) {
  try {
    std::rethrow_exception(exception);
  } catch (const scada::status_exception& e) {
    BOOST_LOG_TRIVIAL(error)
        << "Unhandled exception in Qt event handler"
        << " | Status = " << ToString(e.status());
    client::ReportE2eStatusIfUnset("failure: qt-event");
  } catch (const std::exception& e) {
    BOOST_LOG_TRIVIAL(error)
        << "Unhandled exception in Qt event handler"
        << " | Error = " << e.what();
    client::ReportE2eStatusIfUnset("failure: qt-event");
  } catch (...) {
    BOOST_LOG_TRIVIAL(error) << "Unhandled unknown exception in Qt event handler";
    client::ReportE2eStatusIfUnset("failure: qt-event");
  }
}

[[noreturn]] void OnTerminate() {
  auto exception = std::current_exception();
  if (exception) {
    LogQtEventException(exception);
  } else {
    BOOST_LOG_TRIVIAL(error) << "std::terminate called without an active exception";
    client::ReportE2eStatusIfUnset("failure: terminate");
  }
  std::_Exit(1);
}

class SafeApplication final : public QApplication {
 public:
  using QApplication::QApplication;

  bool notify(QObject* receiver, QEvent* event) override {
    try {
      return QApplication::notify(receiver, event);
    } catch (...) {
      LogQtEventException(std::current_exception());
      return false;
    }
  }
};

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
    std::set_terminate(&OnTerminate);

    GdiplusInitializer gdiplus;

    SafeApplication qapp(argc, argv);

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

    executor->PostTask([&app, executor] {
      auto run_promise = app.Start()
                             .then([&app, executor] {
                               return client::RunE2eObjectViewValuesCheck(
                                   app, executor);
                             })
                             .then([&app] {
                               return client::RunE2eOperatorUseCaseSmoke(app);
                             })
                             .then([&app, executor] {
                               BOOST_LOG_TRIVIAL(info)
                                   << "Client startup completed; entering "
                                      "steady-state run loop";
                               client::ReportE2eStatusIfUnset("success");
                               client::RunE2eObjectTreeLabelsCheck(app, executor)
                                   .then([] {});
                               return app.Run();
                             })
                             .except([](std::exception_ptr exception) {
                               LogStartupException(exception);
                               client::ReportE2eStatusIfUnset("failure: startup");
                             });

      if (client::IsE2eTestMode()) {
        run_promise.then([] {
          BOOST_LOG_TRIVIAL(info)
              << "E2E mode: application run loop completed";
        });
      } else {
        run_promise.then(&QApplication::quit);
      }
    });

    return QApplication::exec();
  } catch (const std::exception&) {
    auto exception = std::current_exception();
    if (auto message = GetStartupErrorMessage(exception)) {
      std::wstring wide_message{message->begin(), message->end()};
      ShowStartupTrace(wide_message.c_str());
      return -1;
    }
    return 0;
  } catch (...) {
    ShowStartupTrace(L"main() failed with an unknown exception");
    return -1;
  }
}
