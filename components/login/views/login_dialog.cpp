#include "components/login/login_dialog.h"

#include "base/strings/sys_string_conversions.h"
#include "common_resources.h"
#include "components/login/login_controller.h"
#include "views/dialog_service_impl_views.h"
#include "views/framework/dialog.h"

#include <atlbase.h>

#include <atlapp.h>
#include <atlctrls.h>
#include <algorithm>

class LoginDialog : public framework::Dialog {
 public:
  explicit LoginDialog(DataServicesContext&& services_context);
  ~LoginDialog();

  DataServices services;

 protected:
  // Dialog
  virtual void OnInitDialog();
  virtual void OnOK();

 private:
  void UpdateControls(bool enable);

  DialogServiceImplViews dialog_service_;
  LoginController controller_;
};

LoginDialog::LoginDialog(DataServicesContext&& services_context)
    : Dialog{IDD_LOGIN},
      controller_{std::move(services_context), dialog_service_} {
  controller_.completion_handler = [this](DataServices& services) {
    this->services = std::move(services);
    EndDialog(IDOK);
  };
  controller_.error_handler = [this] {
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
      .SetCheck(controller_.auto_login ? BST_CHECKED : BST_UNCHECKED);

  // Users.
  {
    WTL::CComboBox user_combo = GetItem(IDC_NAME);
    for (auto& user_name : controller_.user_list)
      user_combo.AddString(user_name.c_str());
    user_combo.SetWindowText(controller_.user_name.c_str());
  }

  SetItemText(IDC_HOST, base::SysNativeMBToWide(controller_.server_host));

  // Server type.
  {
    WTL::CComboBox server_type_combo = GetItem(IDC_SERVER_TYPE);
    int selected_index = 0;
    auto& list = GetDataServicesInfoList();
    for (size_t i = 0; i < list.size(); ++i) {
      auto& info = list[i];
      server_type_combo.AddString(info.display_name.c_str());
      if (EqualDataServicesName(info.name, controller_.server_type))
        selected_index = i;
    }
    server_type_combo.SetCurSel(selected_index);
  }

  UpdateControls(true);
  SetItemText(IDC_STATUS, L"");

  if (controller_.auto_login) {
    SetItemText(IDC_PASSWORD, base::SysNativeMBToWide(controller_.password));
    OnOK();
  }
}

void LoginDialog::OnOK() {
  controller_.user_name = GetItemText(IDC_NAME);
  controller_.password = base::SysWideToNativeMB(GetItemText(IDC_PASSWORD));
  controller_.server_host = base::SysWideToNativeMB(GetItemText(IDC_HOST));
  {
    int server_type_index =
        WTL::CComboBox(GetItem(IDC_SERVER_TYPE)).GetCurSel();
    server_type_index = std::max(server_type_index, 0);
    controller_.server_type = GetDataServicesInfoList()[server_type_index].name;
  }
  controller_.auto_login =
      WTL::CButton(GetItem(IDC_AUTO_LOGIN)).GetState() == BST_CHECKED;

  SetItemText(IDC_STATUS, L"Подключение...");
  UpdateControls(false);
  controller_.Login();
}

bool ExecuteLoginDialog(DataServicesContext&& services_context,
                        DataServices& services) {
  LoginDialog login_dialog{std::move(services_context)};
  if (login_dialog.Execute() != IDOK)
    return false;
  services = login_dialog.services;
  return true;
}
