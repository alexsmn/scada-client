#include "components/limits/limit_dialog.h"

#include "base/strings/string_util.h"
#include "common_resources.h"
#include "components/limits/limit_model.h"
#include "services/dialog_service.h"

#include <atlbase.h>

#include <atlapp.h>
#include <atlstr.h>
#include <atlwin.h>

class LimitsDialog : protected ATL::CDialogImpl<LimitsDialog> {
 public:
  explicit LimitsDialog(LimitModel& model);

  bool Execute(gfx::NativeView parent = nullptr);

 protected:
  friend class ATL::CDialogImpl<LimitsDialog>;

  enum { IDD = IDD_LIMITS };

  BEGIN_MSG_MAP(LimitsDialog)
    MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    COMMAND_ID_HANDLER(IDOK, OnOK)
    COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
  END_MSG_MAP()

  LRESULT OnInitDialog(UINT /*uMsg*/,
                       WPARAM /*wParam*/,
                       LPARAM /*lParam*/,
                       BOOL& /*bHandled*/);
  LRESULT OnOK(WORD /*wNotifyCode*/,
               WORD /*wID*/,
               HWND /*hWndCtl*/,
               BOOL& /*bHandled*/);
  LRESULT OnCancel(WORD /*wNotifyCode*/,
                   WORD /*wID*/,
                   HWND /*hWndCtl*/,
                   BOOL& /*bHandled*/);

 private:
  LimitModel& model_;
};

LimitsDialog::LimitsDialog(LimitModel& model) : model_{model} {}

bool LimitsDialog::Execute(gfx::NativeView parent) {
  return DoModal(parent) == IDOK;
}

LRESULT LimitsDialog::OnInitDialog(UINT /*uMsg*/,
                                   WPARAM /*wParam*/,
                                   LPARAM /*lParam*/,
                                   BOOL&
                                   /*bHandled*/) {
  CenterWindow(GetParent());

  SetDlgItemText(IDC_DESC, base::AsWString(model_.GetSourceTitle()).c_str());

  auto limits = model_.GetLimits();
  SetDlgItemText(IDC_LIMIT_LOLO, base::AsWString(limits.lolo).c_str());
  SetDlgItemText(IDC_LIMIT_HIHI, base::AsWString(limits.hihi).c_str());
  SetDlgItemText(IDC_LIMIT_LO, base::AsWString(limits.lo).c_str());
  SetDlgItemText(IDC_LIMIT_HI, base::AsWString(limits.hi).c_str());

  return TRUE;
}

LRESULT LimitsDialog::OnOK(WORD /*wNotifyCode*/,
                           WORD /*wID*/,
                           HWND /*hWndCtl*/,
                           BOOL&
                           /*bHandled*/) {
  LimitModel::Limits limits = {};
  ATL::CString str;
  GetDlgItemText(IDC_LIMIT_LO, str);
  limits.lo = base::AsString16(static_cast<LPCTSTR>(str));
  GetDlgItemText(IDC_LIMIT_HI, str);
  limits.hi = base::AsString16(static_cast<LPCTSTR>(str));
  GetDlgItemText(IDC_LIMIT_LOLO, str);
  limits.lolo = base::AsString16(static_cast<LPCTSTR>(str));
  GetDlgItemText(IDC_LIMIT_HIHI, str);
  limits.hihi = base::AsString16(static_cast<LPCTSTR>(str));

  model_.WriteLimits(limits);

  EndDialog(IDOK);
  return 0;
}

LRESULT LimitsDialog::OnCancel(WORD /*wNotifyCode*/,
                               WORD /*wID*/,
                               HWND /*hWndCtl*/,
                               BOOL&
                               /*bHandled*/) {
  EndDialog(IDCANCEL);
  return 0;
}

void ShowLimitsDialog(DialogService& dialog_service,
                      LimitDialogContext&& context) {
  LimitModel model{std::move(context)};
  LimitsDialog dialog{model};
  dialog.Execute(dialog_service.GetDialogOwningWindow());
}
