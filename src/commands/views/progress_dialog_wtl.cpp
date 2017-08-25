#include "client/commands/views/progress_dialog.h"

#include <atlbase.h>
#include <wtl/atlapp.h>
#include <atlwin.h>
#include <wtl/atlctrls.h>

#include "client/common_resources.h"

namespace {

class ProgressDialogWtlImpl;

class ProgressDialogWtl : public ATL::CDialogImpl<ProgressDialogWtl> {
 public:
  enum { IDD = IDD_PROGRESS };

  ProgressDialogWtl();
  
 protected:
  virtual void OnFinalMessage(HWND window) override;

  BEGIN_MSG_MAP(ProgressDialog)
    MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
  END_MSG_MAP()

  LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
  LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

 private:
  friend class ProgressDialogWtlImpl;

  WTL::CProgressBarCtrl progress_bar_;
   
  bool cancelled_;
};

class ProgressDialogWtlImpl :  public ProgressDialog {
 public:
  ProgressDialogWtlImpl();
  virtual ~ProgressDialogWtlImpl() override;
  
  // ProgressDialog
  virtual void SetProgress(int range, int position) override;
  virtual void SetStatus(const base::string16& status) override;
  virtual bool IsCancelled() const override;

 private:
  ProgressDialogWtl* dialog_;
};

ProgressDialogWtl::ProgressDialogWtl()
    : cancelled_(false) {
	Create(NULL);
	ShowWindow(SW_SHOW);
}

void ProgressDialogWtl::OnFinalMessage(HWND window) {
	delete this;
}

LRESULT ProgressDialogWtl::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	CenterWindow(GetParent());
	progress_bar_ = GetDlgItem(IDC_PROGRESS);
	return TRUE;
}

LRESULT ProgressDialogWtl::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	cancelled_ = true;
	GetDlgItem(IDCANCEL).EnableWindow(FALSE);
	return 0;
}

// ProgressDialogWtlImpl

ProgressDialogWtlImpl::ProgressDialogWtlImpl()
    : dialog_(new ProgressDialogWtl) {
}

ProgressDialogWtlImpl::~ProgressDialogWtlImpl() {
  if (dialog_)
	  dialog_->DestroyWindow();
}

void ProgressDialogWtlImpl::SetProgress(int range, int position) {
  if (!dialog_)
    return;
	dialog_->progress_bar_.SetRange(0, range);
	dialog_->progress_bar_.SetPos(position);
	dialog_->progress_bar_.UpdateWindow();
}

void ProgressDialogWtlImpl::SetStatus(const base::string16& status) {
	dialog_->SetDlgItemText(IDC_STATUS, status.c_str());
}

bool ProgressDialogWtlImpl::IsCancelled() const {
  return dialog_ && dialog_->cancelled_;
}

} // namespace

std::unique_ptr<ProgressDialog> CreateProgressDialog() {
  return std::unique_ptr<ProgressDialog>(new ProgressDialogWtlImpl);
}