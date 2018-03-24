#pragma once

#include "base/message_loop/message_loop.h"
#include "base/message_loop/message_pump_dispatcher.h"
#include "views/framework/dialog.h"

#include <atlbase.h>
#include <atlwin.h>

namespace framework {

class DialogImplWtl : public DialogImpl,
                      protected ATL::CDialogImpl<DialogImplWtl>,
                      protected base::MessagePumpDispatcher {
 public:
  explicit DialogImplWtl(Dialog& dialog);

  // Dialog
  virtual unsigned Execute(HWND parent);
  virtual void EndDialog(unsigned modal_result);
  
 protected:
  BEGIN_MSG_MAP(DialogImplWtl)
    MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
    COMMAND_ID_HANDLER(IDOK, OnOK)
    COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
    MESSAGE_HANDLER(WM_COMMAND, OnCommand)
  END_MSG_MAP()

  LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
  LRESULT OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
  LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
  LRESULT OnKeyDown(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/);
  LRESULT OnCommand(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);

 protected:
  // ATL::CDialogImpl<DialogImplWtl>
  virtual void OnFinalMessage(HWND /*hWnd*/);

  // MessagePumpDispatcher
  virtual uint32_t Dispatch(const base::NativeEvent& event);

 private:
  friend class ATL::CDialogImpl<DialogImplWtl>;

  unsigned IDD;
  unsigned modal_result_;
  bool executing_;
};

} // namespace framework
