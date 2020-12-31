#include "base/run_loop.h"
#include "views/framework/dialog_impl_wtl.h"
#include "views/framework/widget.h"

namespace framework {

DialogImplWtl::DialogImplWtl(Dialog& dialog)
      : DialogImpl(dialog),
        IDD(dialog.resource_id()),
        executing_(false) {
}

unsigned DialogImplWtl::Execute(HWND parent) {
  assert(!executing_);
  executing_ = true;

  Create(parent);
  ShowWindow(SW_SHOW);

  bool enable_main_window = false;

  HWND main_window = parent ? CWindow(parent).GetTopLevelWindow() : NULL;
  if (main_window && main_window != ::GetDesktopWindow()) {
    ::EnableWindow(main_window, FALSE);
    enable_main_window = true;
  }

  {
    base::MessageLoop::ScopedNestableTaskAllower allow;
    base::RunLoop run_loop(this);

    run_loop.Run();
  }

  if (enable_main_window)
    ::EnableWindow(main_window, TRUE);

  if (main_window && m_hWnd == ::GetActiveWindow())
    ::SetActiveWindow(main_window);

  DestroyWindow();

  return static_cast<unsigned>(modal_result_);
}

void DialogImplWtl::EndDialog(unsigned modal_result) {
  if (executing_) {
    SetWindowPos(NULL, 0, 0, 0, 0,
        SWP_HIDEWINDOW | SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);

    modal_result_ = modal_result;
    executing_ = false;
  }
}

void DialogImplWtl::OnFinalMessage(HWND /*hWnd*/) {
}

LRESULT DialogImplWtl::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
  CenterWindow(GetParent());
  set_window_handle(m_hWnd);
  dialog().OnInitDialog();
  return TRUE;
}

LRESULT DialogImplWtl::OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
  dialog().OnOK();
  return 0;
}

LRESULT DialogImplWtl::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
  dialog().OnCancel();
  return 0;
}

LRESULT DialogImplWtl::OnKeyDown(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
  if (wParam == VK_RETURN && GetFocus() != GetDlgItem(IDCANCEL))
    dialog().OnOK();
  return 0;
}

uint32_t DialogImplWtl::Dispatch(const base::NativeEvent& event) {
  MSG& msg = const_cast<MSG&>(event);
  bool dispatched = !!IsDialogMessage(&msg);
  return !executing_ ? POST_DISPATCH_QUIT_LOOP :
         dispatched ? POST_DISPATCH_NONE :
         POST_DISPATCH_PERFORM_DEFAULT;
}

LRESULT DialogImplWtl::OnCommand(UINT /*uMsg*/, WPARAM wParam,
                                 LPARAM lParam, BOOL& /*bHandled*/) {
  unsigned id = LOWORD(wParam);
  UINT notification_code = HIWORD(wParam);
  // HWND window = (HWND)lParam;
  
  Widget* widget = dialog().GetView(id);
  if (widget)
    widget->OnCommand(notification_code); 
  
  return 0;
}

// Dialog

Dialog::Dialog(unsigned resource_id)
    : resource_id_(resource_id),
      impl_(new DialogImplWtl(*this)) { 
}

} // namespace framework
