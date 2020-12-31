#include "components/main/views/native_main_window.h"

#include "controls/color.h"
#include "command_handler.h"
#include "components/main/views/main_window_views.h"
#include "components/main/views/status_bar_controller.h"
#include "ui/base/models/menu_model.h"
#include "ui/views/widget/widget.h"

NativeMainWindow::NativeMainWindow(NativeMainWindowContext&& context)
    : NativeMainWindowContext{std::move(context)} {
  menu_.SetModel(menu_model_.get());
}

NativeMainWindow::~NativeMainWindow() {}

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
  main_widget_.reset();

  main_window_ = NULL;

  DestroyWindow();
}

HWND NativeMainWindow::Create(HWND hWndParent,
                              ATL::_U_RECT rect,
                              LPCTSTR szWindowName,
                              DWORD dwStyle,
                              DWORD dwExStyle,
                              HMENU hMenu,
                              LPVOID lpCreateParam) {
  menu_.GetNativeMenu();
  menu_.Update();

  // create
  return __super::Create(hWndParent, rect, szWindowName, dwStyle, dwExStyle,
                         menu_.GetNativeMenu(), lpCreateParam);
}

LRESULT NativeMainWindow::OnCreate(UINT /*uMsg*/,
                                   WPARAM /*wParam*/,
                                   LPARAM lParam,
                                   BOOL& /*bHandled*/) {
  assert(main_window_);

  views::Widget::InitParams params;
  params.parent = m_hWnd;
  params.ownership = views::Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET;
  main_widget_.reset(new views::Widget);
  main_widget_->Init(params);
  m_hWndClient = main_widget_->GetNativeView();

  main_widget_->SetContentsView(main_window_);

  CreateSimpleStatusBar();
  status_bar_controller_ =
      std::make_unique<StatusBarController>(status_bar_model_, m_hWndStatusBar);

  UpdateLayout();

  return 0;
}

LRESULT NativeMainWindow::OnDestroy(UINT /*uMsg*/,
                                    WPARAM /*wParam*/,
                                    LPARAM /*lParam*/,
                                    BOOL& bHandled) {
  status_bar_controller_.reset();

  // Suppress WM_QUIT from CFrameWindowImpl handler.
  bHandled = TRUE;
  return DefWindowProc();
}

LRESULT NativeMainWindow::OnClose(UINT /*uMsg*/,
                                  WPARAM /*wParam*/,
                                  LPARAM /*lParam*/,
                                  BOOL& bHandled) {
  assert(main_window_);
  main_window_->Close();
  bHandled = TRUE;
  return 0;
}

LRESULT NativeMainWindow::OnSize(UINT /*uMsg*/,
                                 WPARAM /*wParam*/,
                                 LPARAM lParam,
                                 BOOL& bHandled) {
  status_bar_controller_->Layout();
  bHandled = FALSE;
  return 0;
}

LPARAM NativeMainWindow::OnCommand(UINT uMsg,
                                   WPARAM wParam,
                                   LPARAM lParam,
                                   BOOL& bHandled) {
  if (!main_window_) {
    bHandled = FALSE;
    return 1;
  }

  UINT notification_code = HIWORD(wParam);
  UINT command_id = LOWORD(wParam);

  if (notification_code > 1) {
    bHandled = FALSE;
    return 1;
  }

  if (command_id && main_window_->ExecuteWindowsCommand(command_id))
    return 0;

  bHandled = FALSE;
  return 1;
}

void NativeMainWindow::GetPrefs(gfx::Rect& bounds, bool& maximized) {
  WINDOWPLACEMENT wp = {sizeof(WINDOWPLACEMENT)};
  GetWindowPlacement(&wp);

  bounds = gfx::Rect(wp.rcNormalPosition);
  maximized = IsZoomed() != 0;
}

void NativeMainWindow::UpdateLayout(BOOL bResizeBar) {
  RECT rect = {0};
  GetClientRect(&rect);

  // position bars and offset their dimensions
  UpdateBarsPosition(rect, bResizeBar);

  // resize client window
  if (m_hWndClient != NULL) {
    ::SetWindowPos(m_hWndClient, NULL, rect.left, rect.top,
                   rect.right - rect.left, rect.bottom - rect.top,
                   SWP_NOZORDER | SWP_NOACTIVATE);
  }
}

void NativeMainWindow::UpdateBarsPosition(RECT& rect, BOOL bResizeBars) {
  // resize status bar
  if (m_hWndStatusBar != NULL &&
      ((DWORD)::GetWindowLong(m_hWndStatusBar, GWL_STYLE) & WS_VISIBLE)) {
    if (bResizeBars)
      ::SendMessage(m_hWndStatusBar, WM_SIZE, 0, 0);
    RECT rectSB = {0};
    ::GetWindowRect(m_hWndStatusBar, &rectSB);
    rect.bottom -= rectSB.bottom - rectSB.top;
  }
}

LRESULT NativeMainWindow::OnInitMenuPopup(UINT /*uMsg*/,
                                          WPARAM wParam,
                                          LPARAM lParam,
                                          BOOL& bHandled) {
  HMENU hmenu = reinterpret_cast<HMENU>(wParam);
  // bool is_window_menu = HIWORD(lParam) != 0;
  // int index = LOWORD(lParam);

  if (!::IsMenu(hmenu))
    return 0;

  WTL::CMenuHandle menu(hmenu);

  int pos = 0;
  while (pos < menu.GetMenuItemCount()) {
    UINT id = menu.GetMenuItemID(pos);

    if (id == ID_COLORS) {
      menu.DeleteMenu(pos, MF_BYPOSITION);
      int n = 0;
      for (size_t i = 0; i < aui::GetColorCount(); i++) {
        const auto color_name = aui::GetColorName(i);
        MENUITEMINFO item = {};
        item.cbSize = sizeof(item);
        item.fMask = MIIM_FTYPE | MIIM_ID | MIIM_STRING | MIIM_STATE;
        item.fType = MFT_STRING | MFT_OWNERDRAW;
        item.wID = ID_COLOR_0 + n++;
        item.dwTypeData = const_cast<LPTSTR>(color_name.data());
        item.cch = color_name.size();
        menu.InsertMenuItem(pos++, TRUE, &item);
      }

    } else {
      if (id != 0 && id != static_cast<UINT>(-1)) {
        UINT state = 0;
        auto* handler = commands_.GetCommandHandler(id);
        if (!handler || !handler->IsCommandEnabled(id))
          state |= MFS_DISABLED;
        if (handler && handler->IsCommandChecked(id))
          state |= MFS_CHECKED;

        MENUITEMINFO mii = {sizeof(MENUITEMINFO)};
        mii.fMask = MIIM_STATE;
        mii.fState = state;
        menu.SetMenuItemInfo(pos, TRUE, &mii);
      }

      pos++;
    }
  }

  return 0;
}

LRESULT NativeMainWindow::OnMeasureItem(UINT /*uMsg*/,
                                        WPARAM /*wParam*/,
                                        LPARAM lParam,
                                        BOOL& bHandled) {
  MEASUREITEMSTRUCT* info = (MEASUREITEMSTRUCT*)lParam;
  if (info->itemID >= ID_COLOR_0 &&
      info->itemID < ID_COLOR_0 + aui::GetColorCount()) {
    info->itemWidth = 80;
    return TRUE;
  }
  return FALSE;
}

LRESULT NativeMainWindow::OnDrawItem(UINT /*uMsg*/,
                                     WPARAM /*wParam*/,
                                     LPARAM lParam,
                                     BOOL& bHandled) {
  DRAWITEMSTRUCT* info = (DRAWITEMSTRUCT*)lParam;
  if (info->itemID >= ID_COLOR_0 &&
      info->itemID < ID_COLOR_0 + aui::GetColorCount()) {
    int color_index = info->itemID - ID_COLOR_0;
    WTL::CDCHandle dc(info->hDC);
    int save = dc.SaveDC();
    // background
    int back = (info->itemState & ODS_SELECTED) ? COLOR_HIGHLIGHT : COLOR_MENU;
    int fore =
        (info->itemState & ODS_SELECTED) ? COLOR_HIGHLIGHTTEXT : COLOR_MENUTEXT;
    dc.FillRect(&info->rcItem, GetSysColorBrush(back));
    // color
    RECT rect = info->rcItem;
    InflateRect(&rect, -2, -2);
    rect.right = rect.left + rect.bottom - rect.top;
    dc.FillSolidRect(
        &rect, skia::SkColorToCOLORREF(aui::GetColor(color_index).sk_color()));
    // text
    rect.left = rect.right + 5;  // text relative to icon offset
    rect.right = info->rcItem.right - 2;
    dc.SetBkColor(GetSysColor(back));
    dc.SetTextColor(GetSysColor(fore));
    const auto color_name = aui::GetColorName(color_index);
    dc.DrawText(color_name.data(), color_name.size(), &rect,
                DT_LEFT | DT_SINGLELINE | DT_VCENTER);
    dc.RestoreDC(save);
    return TRUE;
  }
  return FALSE;
}

void NativeMainWindow::SetFlashing(bool flashing) {
  if (flashing_ != flashing) {
    flashing_ = flashing;

    FLASHWINFO flash = {sizeof(FLASHWINFO)};
    flash.hwnd = reinterpret_cast<HWND>(m_hWnd);
    flash.dwFlags = flashing_ ? FLASHW_ALL | FLASHW_TIMER : FLASHW_STOP;
    FlashWindowEx(&flash);
  }
}

void NativeMainWindow::OnFinalMessage(HWND hwnd) {
  delete this;
}
