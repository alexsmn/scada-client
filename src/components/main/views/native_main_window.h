#pragma once

#include <algorithm>
#include <memory>

using std::min;
using std::max;

#include <atlbase.h>
#include <wtl/atlapp.h>
#include <wtl/atlframe.h>

#include "client/common_resources.h"
#include "client/components/main/views/main_menu.h"

namespace gfx {
class Rect;
}

namespace events {
class EventManager;
}

namespace scada {
class SessionService;
}

namespace views {
class Widget;
}

class MainMenu;
class MainWindowViews;
class NodeRefService;
class StatusBarController;

struct NativeMainWindowContext {
  MainWindowViews& main_view_;
  ActionManager& action_manager_;
  events::EventManager& event_manager_;
  Profile& profile_;
  scada::SessionService& session_service_;
  FileCache& file_cache_;
  Favourites& favourites_;
  NodeRefService& node_service_;
};

class NativeMainWindow : public WTL::CFrameWindowImpl<NativeMainWindow>,
                         private NativeMainWindowContext {
 public:
  explicit NativeMainWindow(NativeMainWindowContext&& context);
  virtual ~NativeMainWindow();
  
  void Init(const gfx::Rect& bounds, bool maximized);
  void Close();
  void GetPrefs(gfx::Rect& bounds, bool& maximized);
  
  void UpdateTitle();

  void UpdateLayout(BOOL bResizeBar = TRUE);
  void UpdateBarsPosition(RECT& rect, BOOL bResizeBars = TRUE);

  void SetWindowFlashing(bool flashing);

protected:
  friend class CFrameWindowImpl<NativeMainWindow>;

  DECLARE_FRAME_WND_CLASS(NULL, IDR_MAINFRAME)

  BEGIN_MSG_MAP(NativeMainWindow)
    MESSAGE_HANDLER(WM_CREATE, OnCreate)
    MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
    MESSAGE_HANDLER(WM_SIZE, OnSize)
    MESSAGE_HANDLER(WM_DRAWITEM, OnDrawItem)
    MESSAGE_HANDLER(WM_MEASUREITEM, OnMeasureItem)
    if (main_menu_)
      CHAIN_MSG_MAP_MEMBER((*main_menu_));
    MESSAGE_HANDLER(WM_COMMAND, OnCommand)
    MESSAGE_HANDLER(WM_CLOSE, OnClose);
    CHAIN_MSG_MAP(CFrameWindowImpl<NativeMainWindow>)
    REFLECT_NOTIFICATIONS()
  END_MSG_MAP()

  LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
  LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
  LRESULT OnClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
  LRESULT OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
  LRESULT OnCommand(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
  LRESULT OnDrawItem(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled);
  LRESULT OnMeasureItem(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled);

  HWND Create(HWND hWndParent, ATL::_U_RECT rect, LPCTSTR szWindowName,
              DWORD dwStyle, DWORD dwExStyle, HMENU hMenu, LPVOID lpCreateParam);

  // ATL::CWindow
  virtual void OnFinalMessage(HWND hwnd) override;
              
private:
  bool closed_ = false;
  bool window_flushing_ = false;

  std::unique_ptr<views::Widget> main_widget_;

  std::unique_ptr<MainMenu> main_menu_;

  std::unique_ptr<StatusBarController> status_bar_controller_;
};
