#include "commands/prompt_dialog.h"

#include "common_resources.h"
#include "services/dialog_service.h"
#include "views/client_utils_views.h"
#include "views/framework/dialog.h"

class PromptDialog : public framework::Dialog {
 public:
  PromptDialog() : Dialog(IDD_PROMPT) {}

  bool Execute(HWND parent,
               base::string16& value,
               const base::char16* prompt,
               const base::char16* title) {
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
  base::string16 title_;
  base::string16 prompt_;
  base::string16 value_;
};

bool RunPromptDialog(DialogService& dialog_service,
                     const base::string16& prompt,
                     const base::string16& title,
                     base::string16& value) {
  PromptDialog dlg;
  return dlg.Execute(dialog_service.GetDialogOwningWindow(), value,
                     prompt.c_str(), title.c_str());
}
