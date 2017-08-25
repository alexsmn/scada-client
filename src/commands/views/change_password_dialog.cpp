#include "client/commands/change_password_dialog.h"

#include "base/strings/stringprintf.h"
#include "base/strings/sys_string_conversions.h"
#include "client/views/framework/dialog.h"
#include "client/client_utils.h"
#include "client/common_resources.h"
#include "core/node_management_service.h"
#include "common/node_ref.h"

class ChangePasswordDialog : public framework::Dialog {
 public:
	ChangePasswordDialog() : Dialog(IDD_CHANGE_PASSWORD) {}

	const std::string& current_password() const { return current_password_; }
	const std::string& new_password() const { return new_password_; }

 protected:
	virtual void OnOK() {
		current_password_ = base::SysWideToNativeMB(GetItemText(IDC_CUR_PASSW));
		new_password_ = base::SysWideToNativeMB(GetItemText(IDC_PASSWORD));
		std::string password2 = base::SysWideToNativeMB(GetItemText(IDC_PASSW2));
		if (new_password_ != password2) {
			MessageBox(window_handle(),
          L"Подтвержденный пароль введен неверно.",
          L"Задать пароль", MB_ICONSTOP);
			return;
		}
		Dialog::OnOK();
	}

 private:
	std::string	current_password_;
	std::string	new_password_;
};

void ShowChangePasswordDialog(const NodeRef& user, LocalEvents& local_events, Profile& profile,
    scada::NodeManagementService& node_management_service) {
  ChangePasswordDialog dialog;
  if (dialog.Execute() != IDOK)
    return;

  std::string user_name = user.browse_name();
  node_management_service.ChangeUserPassword(
      user.id(), dialog.current_password(), dialog.new_password(),
      [user_name, &local_events, &profile](const scada::Status& status) {
        base::string16 title = base::StringPrintf(
            L"Смена пароля пользователя %ls",
            base::SysNativeMBToWide(user_name).c_str());
        ReportRequestResult(title, status, local_events, profile);
      });
}
