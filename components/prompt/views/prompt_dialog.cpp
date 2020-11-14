#include "components/prompt/prompt_dialog.h"

#include "common_resources.h"
#include "services/dialog_service.h"
#include "views/client_utils_views.h"
#include "views/framework/dialog.h"

class PromptDialog : public framework::Dialog {
 public:
  PromptDialog() : Dialog(IDD_PROMPT) {}

  bool Execute(HWND parent,
               std::wstring& value,
               const wchar_t* prompt,
               const wchar_t* title) {
    title_ = title;
    prompt_ = prompt;
    value_ = value;
    if (Dialog::Execute(parent) != IDOK)
      return false;
    value = value_;
    return true;
  }

 protected:
  virtual void OnInitDialog() {
    SetWindowText(title_);
    SetItemText(IDC_PROMPT, prompt_);
    SetItemText(IDC_EDIT, value_);
  }

  virtual void OnOK() {
    value_ = GetItemText(IDC_EDIT);
    Dialog::OnOK();
  }

 private:
  std::wstring title_;
  std::wstring prompt_;
  std::wstring value_;
};

bool RunPromptDialog(DialogService& dialog_service,
                     const std::wstring& prompt,
                     const std::wstring& title,
                     std::wstring& value) {
  PromptDialog dlg;
  return dlg.Execute(dialog_service.GetDialogOwningWindow(), value,
                     prompt.c_str(), title.c_str());
}
