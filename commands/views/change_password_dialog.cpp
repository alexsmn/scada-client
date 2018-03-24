#include "commands/change_password_dialog.h"

#include "base/strings/stringprintf.h"
#include "client_utils.h"
#include "common/node_ref.h"
#include "common_resources.h"
#include "core/node_management_service.h"
#include "views/framework/dialog.h"

class ChangePasswordDialog : public framework::Dialog {
 public:
  ChangePasswordDialog() : Dialog{IDD_CHANGE_PASSWORD} {}

  const base::string16& current_password() const { return current_password_; }
  const base::string16& new_password() const { return new_password_; }

 protected:
  virtual void OnOK() {
    current_password_ = GetItemText(IDC_CUR_PASSW);
    new_password_ = GetItemText(IDC_PASSWORD);
    auto password2 = GetItemText(IDC_PASSW2);
    if (new_password_ != password2) {
      MessageBox(window_handle(), L"Подтвержденный пароль введен неверно.",
                 L"Задать пароль", MB_ICONSTOP);
      return;
    }
    Dialog::OnOK();
  }

 private:
  base::string16 current_password_;
  base::string16 new_password_;
};

void ShowChangePasswordDialog(
    const NodeRef& user,
    scada::NodeManagementService& node_management_service,
    LocalEvents& local_events,
    Profile& profile) {
  ChangePasswordDialog dialog;
  if (dialog.Execute() != IDOK)
    return;

  node_management_service.ChangeUserPassword(
      user.id(), dialog.current_password(), dialog.new_password(),
      [user, &local_events, &profile](const scada::Status& status) {
        base::string16 title =
            base::StringPrintf(L"Смена пароля пользователя %ls",
                               ToString16(user.display_name()).c_str());
        ReportRequestResult(title, status, local_events, profile);
      });
}
