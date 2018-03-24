#pragma once

#include <algorithm>
#include <memory>

using std::max;
using std::min;

#include <atlbase.h>

#include <atlapp.h>
#include <atlframe.h>

#include "common_resources.h"
#include "components/main/views/main_menu.h"

namespace gfx {
class Rect;
}

namespace views {
class Widget;
}

class MainMenu;
class MainWindowViews;
class StatusBarController;

struct NativeMainWindowContext {
  MainWindowViews* main_window_;
  MainMenu& main_menu_;
  StatusBarController& status_bar_controller_;
};

class NativeMainWindow : private NativeMainWindowContext,
                         public WTL::CFrameWindowImpl<NativeMainWindow> {
 public:
  explicit NativeMainWindow(NativeMainWindowContext&& context);
  virtual ~NativeMainWindow();

  void Init(const gfx::Rect& bounds, bool maximized);
  void Close();
  void GetPrefs(gfx::Rect& bounds, bool& maximized);

  void UpdateTitle();

  void UpdateLayout(BOOL bResizeBar = TRUE);
  void UpdateBarsPosition(RECT& rect, BOOL bResizeBars = TRUE);

  void SetFlashing(bool flashing);

 protected:
  friend class CFrameWindowImpl<NativeMainWindow>;

  DECLARE_FRAME_WND_CLASS(NULL, IDR_MAINFRAME)

  BEGIN_MSG_MAP(NativeMainWindow)
  MESSAGE_HANDLER(WM_CREATE, OnCreate)
  MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
  MESSAGE_HANDLER(WM_SIZE, OnSize)
  MESSAGE_HANDLER(WM_DRAWITEM, OnDrawItem)
  MESSAGE_HANDLER(WM_MEASUREITEM, OnMeasureItem)
  CHAIN_MSG_MAP_MEMBER(main_menu_);
  MESSAGE_HANDLER(WM_COMMAND, OnCommand)
  MESSAGE_HANDLER(WM_CLOSE, OnClose);
  CHAIN_MSG_MAP(CFrameWindowImpl<NativeMainWindow>)
  REFLECT_NOTIFICATIONS()
  END_MSG_MAP()

  LRESULT OnCreate(UINT /*uMsg*/,
                   WPARAM /*wParam*/,
                   LPARAM /*lParam*/,
                   BOOL& /*bHandled*/);
  LRESULT OnDestroy(UINT /*uMsg*/,
                    WPARAM /*wParam*/,
                    LPARAM /*lParam*/,
                    BOOL& /*bHandled*/);
  LRESULT OnClose(UINT /*uMsg*/,
                  WPARAM /*wParam*/,
                  LPARAM /*lParam*/,
                  BOOL& /*bHandled*/);
  LRESULT OnSize(UINT /*uMsg*/,
                 WPARAM /*wParam*/,
                 LPARAM /*lParam*/,
                 BOOL& /*bHandled*/);
  LRESULT OnCommand(UINT /*uMsg*/,
                    WPARAM /*wParam*/,
                    LPARAM /*lParam*/,
                    BOOL& /*bHandled*/);
  LRESULT OnDrawItem(UINT /*uMsg*/,
                     WPARAM /*wParam*/,
                     LPARAM lParam,
                     BOOL& bHandled);
  LRESULT OnMeasureItem(UINT /*uMsg*/,
                        WPARAM /*wParam*/,
                        LPARAM lParam,
                        BOOL& bHandled);

  HWND Create(HWND hWndParent,
              ATL::_U_RECT rect,
              LPCTSTR szWindowName,
              DWORD dwStyle,
              DWORD dwExStyle,
              HMENU hMenu,
              LPVOID lpCreateParam);

  // ATL::CWindow
  virtual void OnFinalMessage(HWND hwnd) override;

 private:
  bool flashing_ = false;

  std::unique_ptr<views::Widget> main_widget_;
};
