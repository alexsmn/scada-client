#include "base/at_exit.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/win/gdiplus_initializer.h"
#include "components/main/main_window.h"
#include "qt/client_application_qt.h"
#include "qt/message_loop_qt.h"

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpstrCmdLine, int nCmdShow) {
  GdiplusInitializer gdiplus;

  base::AtExitManager at_exit;

  ClientApplicationQt app(0, nullptr);

  // QApplication must be created.
  base::ThreadTaskRunnerHandle message_loop(make_scoped_refptr(new MessageLoopQt));

  if (!app.Init())
    return 1;

  if (!app.ShowLoginDialog())
    return 1;

  app.BeforeRun();

	return app.Run(0);
}
