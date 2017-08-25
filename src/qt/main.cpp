#include "base/at_exit.h"
#include "base/win/gdiplus_initializer.h"
#include "components/main/main_window.h"
#include "qt/client_application_qt.h"
#include "qt/message_loop_qt.h"

int main(int argc, char* argv[]) {
  GdiplusInitializer gdiplus;

  base::AtExitManager at_exit;

  ClientApplicationQt app(argc, argv);

  // QApplication must be created.
  scoped_refptr<MessageLoopQt> message_loop(new MessageLoopQt);

  if (!app.Init())
    return 1;

  if (!app.ShowLoginDialog())
    return 1;

  app.BeforeRun();

	return app.Run(0);
}
