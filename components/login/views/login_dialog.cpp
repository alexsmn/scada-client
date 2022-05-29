#include "components/login/login_dialog.h"

#include "base/strings/string_util.h"
#include "base/strings/sys_string_conversions.h"
#include "common_resources.h"
#include "components/login/login_controller.h"
#include "views/dialog_service_impl_views.h"
#include "views/framework/dialog.h"

#include <atlbase.h>

#include <algorithm>
#include <atlapp.h>
#include <atlctrls.h>

class LoginDialog : public framework::Dialog {
 public:
  LoginDialog(std::shared_ptr<Executor> executor,
              DataServicesContext&& services_context);
  ~LoginDialog();

  DataServices services;

 protected:
  // Dialog
  virtual void OnInitDialog() override;
  virtual void OnOK() override;

 private:
  void UpdateControls(bool enable);

  DialogServiceImplViews dialog_service_;
  const std::shared_ptr<LoginController> controller_;
};

LoginDialog::LoginDialog(std::shared_ptr<Executor> executor,
                         DataServicesContext&& services_context)
    : Dialog{IDD_LOGIN},
      controller_{std::make_shared<LoginController>(std::move(executor),
                                                    std::move(services_context),
                                                    dialog_service_)} {
  controller_->completion_handler = [this](DataServices services) {
    this->services = std::move(services);
    EndDialog(IDOK);
  };
  controller_->error_handler = [this] {
    SetItemText(IDC_STATUS, {});
    UpdateControls(true);
    WTL::CEdit edit = GetItem(IDC_NAME);
    edit.SetSelAll();
    edit.SetFocus();
  };
}

LoginDialog::~LoginDialog() {}

void LoginDialog::UpdateControls(bool enable) {
  int ids[] = {IDOK,     IDC_NAME,        IDC_PASSWORD,
               IDC_HOST, IDC_SERVER_TYPE, IDC_AUTO_LOGIN};
  for (int i = 0; i < _countof(ids); i++)
    EnableWindow(GetItem(ids[i]), enable);
}

void LoginDialog::OnInitDialog() {
  dialog_service_.dialog_owning_window = window_handle();

  WTL::CButton(GetItem(IDC_AUTO_LOGIN))
      .SetCheck(controller_->auto_login ? BST_CHECKED : BST_UNCHECKED);

  // Users.
  {
    WTL::CComboBox user_combo = GetItem(IDC_NAME);
    for (auto& user_name : controller_->user_list)
      user_combo.AddString(base::AsWString(user_name).c_str());
    user_combo.SetWindowText(base::AsWString(controller_->user_name).c_str());
  }

  SetItemText(IDC_HOST, base::SysNativeMBToWide(controller_->server_host));

  // Server type.
  {
    WTL::CComboBox server_type_combo = GetItem(IDC_SERVER_TYPE);
    for (auto& item : controller_->server_type_list)
      server_type_combo.AddString(base::AsWString(item).c_str());
    server_type_combo.SetCurSel(controller_->server_type_index());
  }

  UpdateControls(true);
  SetItemText(IDC_STATUS, L"");

  if (controller_->auto_login) {
    SetItemText(IDC_PASSWORD, base::AsWString(controller_->password));
    OnOK();
  }
}

void LoginDialog::OnOK() {
  controller_->user_name = base::AsString16(GetItemText(IDC_NAME));
  controller_->password = base::AsString16(GetItemText(IDC_PASSWORD));
  controller_->server_host = base::SysWideToNativeMB(GetItemText(IDC_HOST));
  {
    int server_type_index =
        WTL::CComboBox(GetItem(IDC_SERVER_TYPE)).GetCurSel();
    controller_->SetServerTypeIndex(std::max(server_type_index, 0));
  }
  controller_->auto_login =
      WTL::CButton(GetItem(IDC_AUTO_LOGIN)).GetState() == BST_CHECKED;

  SetItemText(IDC_STATUS, L"Подключение...");
  UpdateControls(false);
  controller_->Login();
}

promise<std::optional<DataServices>> ExecuteLoginDialog(
    std::shared_ptr<Executor> executor,
    DataServicesContext&& services_context) {
  LoginDialog login_dialog{std::move(executor), std::move(services_context)};
  if (login_dialog.Execute() != IDOK)
    return make_resolved_promise(std::optional<DataServices>{});
  return make_resolved_promise(
      std::optional<DataServices>{std::move(login_dialog.services)});
}
