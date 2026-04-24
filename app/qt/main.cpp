#include "app/app_init.h"
#include "app/client_application.h"
#include "app/startup_exception.h"
#include "app/qt/installed_style.h"
#include "app/qt/installed_translation.h"
#include "aui/qt/message_loop_qt.h"
#include "base/e2e_test_hooks.h"
#include "base/boost_log.h"
#include "base/executor_timer.h"
#include "base/win/gdiplus_initializer.h"
#include "components/login/login_dialog.h"
#include "controller/window_info.h"
#include "main_window/main_window.h"
#include "main_window/main_window_interface.h"
#include "main_window/main_window_manager.h"
#include "profile/window_definition.h"
#include "project.h"
#include "resources/common_resources.h"
#include "scada/status_exception.h"
#include "services/atl_module.h"

#include <Windows.h>
#include <QApplication>
#include <QSettings>
#include <QTimer>
#include <boost/asio/io_context.hpp>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

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

struct OperatorUseCaseSmokeState {
  bool ok = true;
  std::vector<std::string> lines;
};

struct OperatorUseCaseSmokeCheck {
  std::string_view id;
  std::string_view description;
  std::vector<std::string_view> open_window_types;
  std::vector<std::string_view> registered_window_types;
  std::vector<unsigned> registered_selection_commands;
  std::vector<std::string_view> printable_window_types;
};

void AddSmokeResult(const std::shared_ptr<OperatorUseCaseSmokeState>& state,
                    std::string_view id,
                    bool ok,
                    std::string detail) {
  state->ok = state->ok && ok;
  state->lines.emplace_back(std::string{id} + (ok ? " ok " : " failure ") +
                            std::move(detail));
}

void WriteOperatorUseCaseSmokeReport(
    const std::filesystem::path& path,
    const OperatorUseCaseSmokeState& state) {
  if (path.empty())
    return;

  std::error_code ec;
  if (path.has_parent_path())
    std::filesystem::create_directories(path.parent_path(), ec);

  std::ofstream output{path, std::ios::binary | std::ios::trunc};
  if (!output)
    return;

  output << "operator-use-cases: " << (state.ok ? "ok" : "failure") << "\n";
  for (const auto& line : state.lines)
    output << line << "\n";
}

MainWindowInterface* GetFirstMainWindow(ClientApplication& app) {
  for (auto& main_window : app.main_window_manager().main_windows())
    return &main_window;
  return nullptr;
}

promise<> OpenOperatorWindow(
    ClientApplication& app,
    std::string window_type,
    const std::shared_ptr<OperatorUseCaseSmokeState>& state) {
  auto* main_window = GetFirstMainWindow(app);
  if (!main_window) {
    AddSmokeResult(state, window_type, false, "main window missing");
    return make_resolved_promise();
  }

  const auto* window_info = FindWindowInfoByName(window_type);
  if (!window_info) {
    AddSmokeResult(state, window_type, false, "window type not registered");
    return make_resolved_promise();
  }

  return main_window->OpenView(WindowDefinition{*window_info}, /*activate=*/true)
      .then([state, window_type](OpenedViewInterface* opened_view) {
        AddSmokeResult(state,
                       window_type,
                       opened_view != nullptr,
                       opened_view ? "opened" : "open returned null");
      });
}

promise<> RunE2eOperatorUseCaseSmoke(ClientApplication& app) {
  auto report_path = client::GetE2eOperatorUseCasesReportPath();
  if (report_path.empty())
    return make_resolved_promise();

  auto state = std::make_shared<OperatorUseCaseSmokeState>();
  const std::vector<OperatorUseCaseSmokeCheck> checks{
      {"UC-1", "monitor live values", {"Log"}},
      {"UC-2", "visualise time-series on a graph", {"Graph"}},
      {"UC-3", "view tables summaries and sheets", {"Table", "Summ", "CusTable"}},
      {"UC-4", "acknowledge events and alarms", {"Event"}},
      {"UC-5", "browse event journals", {"EventJournal"}},
      {"UC-6", "watch a custom spreadsheet", {"CusTable"}},
      {"UC-7", "issue control commands", {}, {}, {ID_WRITE, ID_WRITE_MANUAL}},
      {"UC-8", "manage favourites and portfolios", {"Favorites", "Portfolio"}},
      {"UC-9", "print or export the active view", {}, {}, {}, {"Table", "EventJournal"}},
      {"UC-10", "browse server files", {"FileSystemView"}},
      {"UC-11", "view Modus and Vidicon schematics", {}, {"Modus", "VidiconDisplay"}},
  };

  promise<> sequence = make_resolved_promise();

  for (const auto& check : checks) {
    for (std::string_view window_type : check.open_window_types) {
      sequence = sequence.then(
          [&app, state, window_type = std::string{window_type}] {
            return OpenOperatorWindow(app, window_type, state);
          });
    }

    sequence = sequence.then([&app, state, check] {
      bool ok = true;
      std::string detail{check.description};

      for (std::string_view window_type : check.registered_window_types) {
        bool registered = FindWindowInfoByName(window_type) != nullptr;
        ok = ok && registered;
        detail += registered ? " registered " : " missing ";
        detail += window_type;
      }

      for (unsigned command_id : check.registered_selection_commands) {
        bool registered = app.HasSelectionCommandForTesting(command_id);
        ok = ok && registered;
        detail += registered ? " command " : " missing-command ";
        detail += std::to_string(command_id);
      }

      for (std::string_view window_type : check.printable_window_types) {
        const auto* window_info = FindWindowInfoByName(window_type);
        bool printable = window_info && window_info->printable();
        ok = ok && printable;
        detail += printable ? " printable " : " not-printable ";
        detail += window_type;
      }

      AddSmokeResult(state, check.id, ok, std::move(detail));
    });
  }

  return sequence.then([report_path, state] {
    WriteOperatorUseCaseSmokeReport(report_path, *state);
  });
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
                             .then([&app] {
                               return RunE2eOperatorUseCaseSmoke(app);
                             })
                             .then([&app] {
                               BOOST_LOG_TRIVIAL(info)
                                   << "Client startup completed; entering "
                                      "steady-state run loop";
                               client::ReportE2eStatusIfUnset("success");
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
