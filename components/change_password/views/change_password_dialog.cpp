#include "components/change_password/change_password_dialog.h"

#include "base/strings/string_util.h"
#include "common_resources.h"
#include "components/change_password/change_password.h"
#include "views/framework/dialog.h"

class ChangePasswordDialog : public framework::Dialog {
 public:
  ChangePasswordDialog() : Dialog{IDD_CHANGE_PASSWORD} {}

  const std::u16string& current_password() const { return current_password_; }
  const std::u16string& new_password() const { return new_password_; }

 protected:
  virtual void OnOK() override {
    current_password_ = base::AsString16(GetItemText(IDC_CUR_PASSW));
    new_password_ = base::AsString16(GetItemText(IDC_PASSWORD));
    auto password2 = base::AsString16(GetItemText(IDC_PASSW2));
    if (new_password_ != password2) {
      MessageBox(window_handle(), L"Подтвержденный пароль введен неверно.",
                 L"Задать пароль", MB_ICONSTOP);
      return;
    }

    Dialog::OnOK();
  }

 private:
  std::u16string current_password_;
  std::u16string new_password_;
};

void ShowChangePasswordDialog(DialogService& dialog_service,
                              ChangePasswordContext&& context) {
  ChangePasswordDialog dialog;
  if (dialog.Execute() != IDOK)
    return;

  ChangePassword(std::move(context), dialog.current_password(),
                 dialog.new_password());
}
