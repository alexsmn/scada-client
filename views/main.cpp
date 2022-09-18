#include "base/at_exit.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/task_runner_executor.h"
#include "base/win/gdiplus_initializer.h"
#include "client_application.h"
#include "components/login/login_dialog.h"
#include "components/main/views/main_window_views.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/views/focus/accelerator_handler.h"
#include "views/activex_host.h"

#include <atlbase.h>
#include <boost/asio/io_context.hpp>

#include <atlapp.h>

CAppModule _Module;

int Run(int show = SW_SHOWDEFAULT) {
  base::AtExitManager at_exit;

  GdiplusInitializer gdiplus;
  ui::ResourceBundle::InitSharedInstance();

  boost::asio::io_context io_context;

  int result = 0;

  try {
    base::MessageLoop message_loop(base::MessageLoop::TYPE_UI);
    base::RunLoop run_loop(&ActiveXHost::instance());

    auto executor =
        std::make_shared<TaskRunnerExecutor>(message_loop.task_runner());
    ClientApplication app{ClientApplicationContext{
        io_context, executor,
        [](MainWindowContext&& context) {
          return std::make_unique<MainWindowViews>(std::move(context));
        },
        [executor](DataServicesContext&& services_context) {
          return ExecuteLoginDialog(executor, std::move(services_context));
        },
        [&run_loop] { run_loop.Quit(); }}};

    Dispatch(*executor, [&app] { app.Start(); });

    views::AcceleratorHandler accelerator_handler;
    ActiveXHost::instance().AddMessageDispatcher(accelerator_handler);

    run_loop.Run();

    ActiveXHost::instance().RemoveMessageDispatcher(accelerator_handler);

  } catch (const std::exception&) {
    result = -1;
  }

  ui::ResourceBundle::CleanupSharedInstance();
  _Module.RemoveMessageLoop();

  return result;
}

int WINAPI _tWinMain(HINSTANCE hInstance,
                     HINSTANCE /*hPrevInstance*/,
                     LPTSTR lpstrCmdLine,
                     int nCmdShow) {
  setlocale(LC_ALL, "Russian");

  HRESULT hRes = ::CoInitialize(NULL);
  ATLASSERT(SUCCEEDED(hRes));

  hRes = ::OleInitialize(NULL);  // for drag& drop
  ATLASSERT(SUCCEEDED(hRes));

  // This resolves ATL window thunking problem when Microsoft Layer for
  // Unicode (MSLU) is used.
  ::DefWindowProc(NULL, 0, 0, 0L);

  // add flags to support other controls
  AtlInitCommonControls(ICC_COOL_CLASSES | ICC_BAR_CLASSES);

  hRes = _Module.Init(nullptr, hInstance);
  ATLASSERT(SUCCEEDED(hRes));

  auto nRet = Run(nCmdShow);

  _Module.Term();
  ::OleUninitialize();
  ::CoUninitialize();

  return nRet;
}
