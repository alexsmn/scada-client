#include "components/prompt/prompt_dialog.h"

#include "base/strings/string_util.h"
#include "common_resources.h"
#include "services/dialog_service.h"
#include "views/client_utils_views.h"
#include "views/framework/dialog.h"

class PromptDialog : public framework::Dialog {
 public:
  PromptDialog() : Dialog(IDD_PROMPT) {}

  std::wstring title;
  std::wstring prompt;
  std::wstring value;

 protected:
  virtual void OnInitDialog() override {
    SetWindowText(title);
    SetItemText(IDC_PROMPT, prompt);
    SetItemText(IDC_EDIT, value);
  }

  virtual void OnOK() override {
    value = GetItemText(IDC_EDIT);
    Dialog::OnOK();
  }
};

bool RunPromptDialog(DialogService& dialog_service,
                     const std::u16string& prompt,
                     const std::u16string& title,
                     std::u16string& value) {
  PromptDialog prompt_dialog;
  prompt_dialog.title = base::AsWString(title);
  prompt_dialog.prompt = base::AsWString(prompt);
  prompt_dialog.value = base::AsWString(value);
  if (prompt_dialog.Execute(dialog_service.GetDialogOwningWindow()) != IDOK)
    return false;
  value = base::AsString16(prompt_dialog.value);
  return true;
}
