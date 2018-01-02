#include <atlbase.h>
#include <atlapp.h>

#include "base/at_exit.h"
#include "base/message_loop/message_loop.h"
#include "base/win/gdiplus_initializer.h"
#include "views/client_application_views.h"
#include "ui/base/resource/resource_bundle.h"

CAppModule _Module;

int Run(int show = SW_SHOWDEFAULT) {
  base::AtExitManager at_exit;

  GdiplusInitializer gdiplus;
  ui::ResourceBundle::InitSharedInstance();

  base::MessageLoop message_loop(base::MessageLoop::TYPE_UI);

  // Message loop must exist on destruction.
  ClientApplicationViews app(0, nullptr);

  if (!app.Init()) {
    ui::ResourceBundle::CleanupSharedInstance();
    _Module.RemoveMessageLoop();
    return -1;
  }
  
  if (!app.ShowLoginDialog()) {
    ui::ResourceBundle::CleanupSharedInstance();
    _Module.RemoveMessageLoop();
    return -1;
  }

  app.BeforeRun();
  
  int res = app.Run(show);

  ui::ResourceBundle::CleanupSharedInstance();

  return res;
}

void ShutdownComModule() {
  _Module.RevokeClassObjects();
}

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/,
                     LPTSTR lpstrCmdLine, int nCmdShow) {
  setlocale(LC_ALL, "Russian");

  HRESULT hRes = ::CoInitialize(NULL);
  ATLASSERT(SUCCEEDED(hRes));

  hRes = ::OleInitialize(NULL);	// for drag& drop
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
