#include "client/components/main/views/native_main_window.h"

#include "base/win/win_util.h"
#include "client/views/client_utils_views.h"
#include "client/window_info.h"
#include "client/base/color.h"
#include "client/components/main/views/main_window_views.h"
#include "client/components/main/views/status_bar_controller.h"
#include "client/components/main/view_manager.h"
#include "client/services/profile.h"
#include "skia/ext/skia_utils_win.h"
#include "ui/views/widget/widget.h"
#include "core/session_service.h"

inline unsigned GetMenuID(scada::SessionService& session_state) {
  return session_state.IsAdministrator() ? IDR_MAINFRAME : IDR_MAIN_USER;
}

NativeMainWindow::NativeMainWindow(NativeMainWindowContext&& context)
    : NativeMainWindowContext(std::move(context)) {
}

NativeMainWindow::~NativeMainWindow() {
}

void NativeMainWindow::Init(const gfx::Rect& bounds, bool maximized) {
  // prepare position
  RECT rect = rcDefault;
  if (!bounds.IsEmpty()) {
    rect = bounds.ToRECT();
    RECT area;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &area, 0);
    OffsetRect(&rect, area.left, area.top);
  }

  if (!CreateEx(NULL, rect))
    throw std::exception("Can't create main window");

//  SetMenu(main_menu_->GetMenuHandle());

  ShowWindow(maximized ? SW_SHOWMAXIMIZED : SW_SHOW);
}

void NativeMainWindow::Close() {
  status_bar_controller_.reset();
  main_menu_.reset();

  main_widget_.reset();

  closed_ = true;

  DestroyWindow();
}

HWND NativeMainWindow::Create(HWND hWndParent, ATL::_U_RECT rect, LPCTSTR szWindowName,
                        DWORD dwStyle, DWORD dwExStyle, HMENU hMenu,
                        LPVOID lpCreateParam) {
  // prepare menu
  CMenuHandle menu;
  menu.LoadMenu(GetMenuID(session_service_));
  
  // create
  return __super::Create(hWndParent, rect, szWindowName, dwStyle, dwExStyle, menu, lpCreateParam);
}

LRESULT NativeMainWindow::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
  views::Widget::InitParams params;
  params.parent = m_hWnd;
  params.ownership = views::Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET;
  main_widget_.reset(new views::Widget);
  main_widget_->Init(params);
  m_hWndClient = main_widget_->GetNativeView();

  main_widget_->SetContentsView(&main_view_);

  main_menu_ = std::make_unique<MainMenu>(main_view_, GetMenu(), action_manager_, profile_, file_cache_, favourites_);

  CreateSimpleStatusBar();
  status_bar_controller_ = std::make_unique<StatusBarController>(m_hWndStatusBar, node_service_, event_manager_, session_service_);

  UpdateLayout();

  return 0;
}

LRESULT NativeMainWindow::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
  // Suppress WM_QUIT from CFrameWindowImpl handler.
  bHandled = TRUE;  
  return DefWindowProc();
}

LRESULT NativeMainWindow::OnClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
  main_view_.Close();
  bHandled = TRUE;
  return 0;
}

LRESULT NativeMainWindow::OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled) {
  status_bar_controller_->Layout();
  bHandled = FALSE;
  return 0;
}

void NativeMainWindow::UpdateTitle() {
  auto title = main_view_.GetWindowTitle();
  SetWindowText(title.c_str());
}

LPARAM NativeMainWindow::OnCommand(UINT uMsg, WPARAM wParam, LPARAM lParam,
                             BOOL& bHandled) {
  if (closed_) {
    bHandled = FALSE;
    return 1;
  }

  UINT notification_code = HIWORD(wParam);
  UINT command_id = LOWORD(wParam);

  if (notification_code > 1) {
    bHandled = FALSE;
    return 1;
  }

  if (command_id && main_view_.ExecuteWindowsCommand(command_id))
    return 0;
  
  bHandled = FALSE;
  return 1;
}

void NativeMainWindow::GetPrefs(gfx::Rect& bounds, bool& maximized) {
  WINDOWPLACEMENT wp = { sizeof(WINDOWPLACEMENT) };
  GetWindowPlacement(&wp);

  bounds = gfx::Rect(wp.rcNormalPosition);
  maximized = IsZoomed() != 0;
}

void NativeMainWindow::UpdateLayout(BOOL bResizeBar) {
  RECT rect = { 0 };
  GetClientRect(&rect);

  // position bars and offset their dimensions
  UpdateBarsPosition(rect, bResizeBar);

  // resize client window
  if(m_hWndClient != NULL) {
    ::SetWindowPos(m_hWndClient, NULL, rect.left, rect.top,
                   rect.right - rect.left, rect.bottom - rect.top,
                   SWP_NOZORDER | SWP_NOACTIVATE);
  }
}

void NativeMainWindow::UpdateBarsPosition(RECT& rect, BOOL bResizeBars) {
  // resize status bar
  if(m_hWndStatusBar != NULL && ((DWORD)::GetWindowLong(m_hWndStatusBar, GWL_STYLE) & WS_VISIBLE))
  {
    if(bResizeBars)
      ::SendMessage(m_hWndStatusBar, WM_SIZE, 0, 0);
    RECT rectSB = { 0 };
    ::GetWindowRect(m_hWndStatusBar, &rectSB);
    rect.bottom -= rectSB.bottom - rectSB.top;
  }
}

LRESULT NativeMainWindow::OnMeasureItem(UINT /*uMsg*/, WPARAM /*wParam*/,
                                  LPARAM lParam, BOOL& bHandled) {
  MEASUREITEMSTRUCT* info = (MEASUREITEMSTRUCT*)lParam;
  if (info->itemID >= ID_COLOR_0 && info->itemID < ID_COLOR_0 + palette::GetColorCount()) {
    info->itemWidth = 80;
    return TRUE;
  }
  return FALSE;
}

LRESULT NativeMainWindow::OnDrawItem(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam,
                               BOOL& bHandled) {
  DRAWITEMSTRUCT* info = (DRAWITEMSTRUCT*)lParam;
  if (info->itemID >= ID_COLOR_0 && info->itemID < ID_COLOR_0 + palette::GetColorCount()) {
    int color_index = info->itemID - ID_COLOR_0;
    WTL::CDCHandle dc(info->hDC);
    int save = dc.SaveDC();
    // background
    int back = (info->itemState & ODS_SELECTED) ?  COLOR_HIGHLIGHT : COLOR_MENU;
    int fore = (info->itemState & ODS_SELECTED) ?  COLOR_HIGHLIGHTTEXT : COLOR_MENUTEXT;
    dc.FillRect(&info->rcItem, GetSysColorBrush(back));
    // color
    RECT rect = info->rcItem;
    InflateRect(&rect, -2, -2);
    rect.right = rect.left + rect.bottom - rect.top;
    dc.FillSolidRect(&rect, skia::SkColorToCOLORREF(palette::GetColor(color_index)));
    // text
    rect.left = rect.right + 5;	// text relative to icon offset
    rect.right = info->rcItem.right - 2;
    dc.SetBkColor(GetSysColor(back));
    dc.SetTextColor(GetSysColor(fore));
    dc.DrawText(palette::GetColorName(color_index), -1, &rect,
                DT_LEFT | DT_SINGLELINE | DT_VCENTER);
    dc.RestoreDC(save);
    return TRUE;
  }
  return FALSE;
}

void NativeMainWindow::SetWindowFlashing(bool flashing) {
  if (window_flushing_ != flashing) {
    window_flushing_ = flashing;

    FLASHWINFO flash = { sizeof(FLASHWINFO) };
    flash.hwnd = reinterpret_cast<HWND>(m_hWnd);
    flash.dwFlags = window_flushing_ ? FLASHW_ALL|FLASHW_TIMER : FLASHW_STOP;
    FlashWindowEx(&flash);
  }
}

void NativeMainWindow::OnFinalMessage(HWND hwnd) {
  delete this;
}
